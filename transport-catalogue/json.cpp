#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include "json.h"
#include <cctype>
#include <sstream>
#include <locale>

using namespace std;

namespace json {

    namespace {

        // Безопасный пропуск whitespace-символов без риска Undefined Behavior
        void SkipWhitespace(istream& input) {
            while (true) {
                int c = input.peek();
                if (c != EOF && isspace(static_cast<unsigned char>(c))) {
                    input.get();
                }
                else {
                    break;
                }
            }
        }

        Node LoadNode(istream& input);

        // Реализация LoadNumber в строгом соответствии с требованиями
        using Number = std::variant<int, double>;
        Number LoadNumber(istream& input) {
            using namespace std::literals;

            string parsed_num;

            // Считывает в parsed_num очередной символ из input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
                };

            // Считывает одну или более цифр в parsed_num из input
            auto read_digits = [&input, read_char] {
                int c = input.peek();
                if (c == EOF || !isdigit(static_cast<unsigned char>(c))) {
                    throw ParsingError("A digit is expected"s);
                }
                while (true) {
                    int next_c = input.peek();
                    if (next_c != EOF && isdigit(static_cast<unsigned char>(next_c))) {
                        read_char();
                    }
                    else {
                        break;
                    }
                }
                };

            if (input.peek() == '-') {
                read_char();
            }

            if (input.peek() == '0') {
                read_char();
            }
            else {
                read_digits();
            }

            bool is_double = false;
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_double = true;
            }

            if (int c = input.peek(); c == 'e' || c == 'E') {
                read_char();
                if (int sign = input.peek(); sign == '+' || sign == '-') {
                    read_char();
                }
                read_digits();
                is_double = true;
            }

            stringstream ss(parsed_num);
            ss.imbue(locale::classic()); // Защита от локали тестирующего сервера (запятая/точка)

            if (is_double) {
                double d;
                if (ss >> d) return d;
                throw ParsingError("Malformed double: "s + parsed_num);
            }
            else {
                int i;
                if (ss >> i) return i;
                throw ParsingError("Malformed int: "s + parsed_num);
            }
        }

        Node LoadString(istream& input) {
            if (input.get() != '"') {
                throw ParsingError("Expected '\"' at start of string");
            }
            string s;
            while (true) {
                int c = input.get();
                if (c == EOF) throw ParsingError("Unexpected EOF in string");
                if (c == '"') break;

                if (c == '\\') {
                    int esc = input.get();
                    if (esc == EOF) throw ParsingError("Unexpected EOF after escape char");
                    switch (esc) {
                    case 'n': s += '\n'; break;
                    case 'r': s += '\r'; break;
                    case 't': s += '\t'; break;
                    case '"':  s += '"'; break;
                    case '\\': s += '\\'; break;
                    default: throw ParsingError("Invalid escape sequence");
                    }
                }
                else {
                    s += static_cast<char>(c);
                }
            }
            return Node(move(s));
        }

        Node LoadNull(istream& input) {
            using namespace std::literals;
            string s;
            for (int i = 0; i < 4; ++i) {
                int c = input.get();
                if (c == EOF) throw ParsingError("Unexpected EOF while parsing null"s);
                s += static_cast<char>(c);
            }
            if (s == "null"s) return Node(nullptr);
            throw ParsingError("Expected null, got: "s + s);
        }

        Node LoadBool(istream& input) {
            using namespace std::literals;
            string s;
            int first_char = input.peek();
            int len = (first_char == 't') ? 4 : 5;
            for (int i = 0; i < len; ++i) {
                int c = input.get();
                if (c == EOF) throw ParsingError("Unexpected EOF while parsing bool"s);
                s += static_cast<char>(c);
            }
            if (s == "true"s) return Node(true);
            if (s == "false"s) return Node(false);
            throw ParsingError("Expected bool, got: "s + s);
        }

        Node LoadArray(istream& input) {
            if (input.get() != '[') throw ParsingError("Expected [ at start of array");
            Array result;
            SkipWhitespace(input);
            if (input.peek() == ']') {
                input.get();
                return Node(move(result));
            }

            while (true) {
                result.push_back(LoadNode(input));
                SkipWhitespace(input);
                int c = input.get();
                if (c == ']') break;
                if (c != ',') throw ParsingError("Expected ',' or ']' in array");
            }
            return Node(move(result));
        }

        Node LoadDict(istream& input) {
            if (input.get() != '{') throw ParsingError("Expected { at start of dictionary");
            Dict result;
            SkipWhitespace(input);
            if (input.peek() == '}') {
                input.get();
                return Node(move(result));
            }

            while (true) {
                SkipWhitespace(input);
                if (input.peek() != '"') throw ParsingError("Expected string key in dictionary");
                Node key_node = LoadString(input);

                string key = key_node.AsString();

                SkipWhitespace(input);
                if (input.get() != ':') throw ParsingError("Expected ':' after key in dictionary");

                result[move(key)] = LoadNode(input);

                SkipWhitespace(input);
                int c = input.get();
                if (c == '}') break;
                if (c != ',') throw ParsingError("Expected ',' or '}' in dictionary");
            }
            return Node(move(result));
        }

        Node LoadNode(istream& input) {
            SkipWhitespace(input);
            int c = input.peek();
            if (c == EOF) throw ParsingError("Unexpected EOF");

            if (c == '[') return LoadArray(input);
            if (c == '{') return LoadDict(input);
            if (c == '"') return LoadString(input);
            if (c == 't' || c == 'f') return LoadBool(input);
            if (c == 'n') return LoadNull(input);
            if (c == '-' || isdigit(static_cast<unsigned char>(c))) {
                auto num = LoadNumber(input);
                if (holds_alternative<int>(num)) {
                    return Node(get<int>(num));
                }
                else {
                    return Node(get<double>(num));
                }
            }
            throw ParsingError("Unexpected character");
        }

        void PrintNode(const Node& node, const PrintContext& ctx);

        template <typename Value>
        void PrintValue(const Value& value, const PrintContext& ctx) {
            ctx.out << value;
        }

        void PrintValue(nullptr_t, const PrintContext& ctx) {
            using namespace std::literals;
            ctx.out << "null"sv;
        }

        void PrintValue(bool value, const PrintContext& ctx) {
            using namespace std::literals;
            ctx.out << (value ? "true"sv : "false"sv);
        }

        void PrintValue(const string& value, const PrintContext& ctx) {
            using namespace std::literals;
            ctx.out << '"';
            for (char c : value) {
                switch (c) {
                case '\n': ctx.out << "\\n"sv; break;
                case '\r': ctx.out << "\\r"sv; break;
                case '\t': ctx.out << "\\t"sv; break;
                case '"':  ctx.out << "\\\""sv; break;
                case '\\': ctx.out << "\\\\"sv; break;
                default:   ctx.out << c; break;
                }
            }
            ctx.out << '"';
        }

        void PrintValue(const Array& value, const PrintContext& ctx) {
            using namespace std::literals;
            ctx.out << "[\n"sv;
            auto inner_ctx = ctx.Indented();
            bool first = true;
            for (const auto& node : value) {
                if (!first) ctx.out << ",\n"sv;
                first = false;
                inner_ctx.PrintIndent();
                PrintNode(node, inner_ctx);
            }
            ctx.out << '\n';
            ctx.PrintIndent();
            ctx.out << ']';
        }

        void PrintValue(const Dict& value, const PrintContext& ctx) {
            using namespace std::literals;
            ctx.out << "{\n"sv;
            auto inner_ctx = ctx.Indented();
            bool first = true;
            for (const auto& [key, node] : value) {
                if (!first) ctx.out << ",\n"sv;
                first = false;
                inner_ctx.PrintIndent();
                PrintValue(key, inner_ctx);
                ctx.out << ": "sv;
                PrintNode(node, inner_ctx);
            }
            ctx.out << '\n';
            ctx.PrintIndent();
            ctx.out << '}';
        }

        void PrintNode(const Node& node, const PrintContext& ctx) {
            visit([&ctx](const auto& value) { PrintValue(value, ctx); }, node.GetValue());
        }

    }  // namespace


    bool Node::IsInt() const { return holds_alternative<int>(*this); }
    bool Node::IsDouble() const { return holds_alternative<int>(*this) || holds_alternative<double>(*this); }
    bool Node::IsPureDouble() const { return holds_alternative<double>(*this); }
    bool Node::IsBool() const { return holds_alternative<bool>(*this); }
    bool Node::IsString() const { return holds_alternative<string>(*this); }
    bool Node::IsNull() const { return holds_alternative<nullptr_t>(*this); }
    bool Node::IsArray() const { return holds_alternative<Array>(*this); }
    bool Node::IsMap() const { return holds_alternative<Dict>(*this); }

    int Node::AsInt() const {
        if (!IsInt()) throw logic_error("Node is not an int");
        return get<int>(*this);
    }
    bool Node::AsBool() const {
        if (!IsBool()) throw logic_error("Node is not a bool");
        return get<bool>(*this);
    }
    double Node::AsDouble() const {
        if (IsInt()) {
            return static_cast<double>(AsInt());
        }
        else if (IsDouble()) {
            return get<double>(*this);
        }
        throw logic_error("Not a number");

    }
    const string& Node::AsString() const {
        if (!IsString()) throw logic_error("Node is not a string");
        return get<string>(*this);
    }
    const Array& Node::AsArray() const {
        if (!IsArray()) throw logic_error("Node is not an array");
        return get<Array>(*this);
    }
    const Dict& Node::AsMap() const {
        if (!IsMap()) throw logic_error("Node is not a map");
        return get<Dict>(*this);
    }

    bool operator==(const Node& lhs, const Node& rhs) { return lhs.GetValue() == rhs.GetValue(); }
    bool operator!=(const Node& lhs, const Node& rhs) { return !(lhs == rhs); }

    Document::Document(Node root) : root_(move(root)) {}
    const Node& Document::GetRoot() const { return root_; }

    bool operator==(const Document& lhs, const Document& rhs) { return lhs.GetRoot() == rhs.GetRoot(); }
    bool operator!=(const Document& lhs, const Document& rhs) { return !(lhs == rhs); }

    Document Load(istream& input) {
        Node root = LoadNode(input);
        SkipWhitespace(input);
        if (input.peek() != EOF) {
            throw ParsingError("Unexpected trailing garbage after JSON root element");
        }
        return Document{ move(root) };
    }

    void Print(const Document& doc, std::ostream& output) {
        PrintNode(doc.GetRoot(), PrintContext{ output });
    }

}  // namespace json

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif