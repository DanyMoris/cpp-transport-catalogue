#pragma once
#include "json.h"
#include <string>
#include <vector>
#include <optional>

namespace json {
    class Builder;
    class DictItemContext;
    class DictKeyContext;
    class ArrayItemContext;

    class BuilderContext {
    protected:
        Builder& builder_;
    public:
        explicit BuilderContext(Builder& builder) : builder_(builder) {}
    };

    class DictItemContext : public BuilderContext {
    public:
        explicit DictItemContext(Builder& builder) : BuilderContext(builder) {}
        DictKeyContext Key(std::string);
        Builder& EndDict();
    };

    class DictKeyContext : public BuilderContext {
    public:
        explicit DictKeyContext(Builder& builder) : BuilderContext(builder) {}
        DictItemContext Value(Node node);
        DictItemContext StartDict();
        ArrayItemContext StartArray();
    };

    class ArrayItemContext : public BuilderContext {
    public:
        explicit ArrayItemContext(Builder& builder) : BuilderContext(builder) {}
        ArrayItemContext Value(Node node);
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        Builder& EndArray();
    };

    class Builder {
    public:
        Builder();

        DictKeyContext Key(std::string key);
        Builder& Value(Node node); // Оставляем Builder& для вызова в корне
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        Builder& EndDict();
        Builder& EndArray();
        Node Build();
    private:
        Node root_;
        std::vector<Node*> nodes_stack_;
        std::optional<std::string> key_;
        bool root_initialized_ = false;

        Node* GetCurrentNode();
    };

    inline DictKeyContext DictItemContext::Key(std::string key) {
        return builder_.Key(std::move(key));
    }
    inline Builder& DictItemContext::EndDict() {
        return builder_.EndDict();
    }

    inline DictItemContext DictKeyContext::Value(Node node) {
        return DictItemContext(builder_.Value(node.GetValue()));
    }
    inline DictItemContext DictKeyContext::StartDict() {
        return builder_.StartDict();
    }
    inline ArrayItemContext DictKeyContext::StartArray() {
        return builder_.StartArray();
    }

    inline ArrayItemContext ArrayItemContext::Value(Node node) {
        return ArrayItemContext(builder_.Value(node.GetValue()));
    }
    inline DictItemContext ArrayItemContext::StartDict() {
        return builder_.StartDict();
    }
    inline ArrayItemContext ArrayItemContext::StartArray() {
        return builder_.StartArray();
    }
    inline Builder& ArrayItemContext::EndArray() {
        return builder_.EndArray();
    }
}