#pragma once
#include "json.h"
#include <string>
#include <vector>
#include <optional>

namespace json {

    class Builder {
    public:

        class BaseContext;
        class DictItemContext;
        class DictKeyContext;
        class ArrayItemContext;

        Builder();

        DictKeyContext Key(std::string key);
        Builder& Value(Node node);
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

    class Builder::BaseContext {
    protected:
        Builder& builder_;
    public:
        explicit BaseContext(Builder& builder) : builder_(builder) {}

        DictKeyContext Key(std::string key);
        Builder& Value(Node node);
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        Builder& EndDict();
        Builder& EndArray();
        Node Build();
    };

    class Builder::DictItemContext : public BaseContext {
    public:
        explicit DictItemContext(Builder& builder) : BaseContext(builder) {}

        Builder& Value(Node node) = delete;
        DictItemContext StartDict() = delete;
        ArrayItemContext StartArray() = delete;
        Builder& EndArray() = delete;
        Node Build() = delete;
    };

    class Builder::DictKeyContext : public BaseContext {
    public:
        explicit DictKeyContext(Builder& builder) : BaseContext(builder) {}

        DictItemContext Value(Node node);

        DictKeyContext Key(std::string key) = delete;
        Builder& EndDict() = delete;
        Builder& EndArray() = delete;
        Node Build() = delete;
    };

    class Builder::ArrayItemContext : public BaseContext {
    public:
        explicit ArrayItemContext(Builder& builder) : BaseContext(builder) {}

        ArrayItemContext Value(Node node);

        DictKeyContext Key(std::string key) = delete;
        Builder& EndDict() = delete;
        Node Build() = delete;
    };

    // --- Реализация методов BaseContext ---
    inline Builder::DictKeyContext Builder::BaseContext::Key(std::string key) {
        return builder_.Key(std::move(key));
    }
    inline Builder& Builder::BaseContext::Value(Node node) {
        return builder_.Value(std::move(node));
    }
    inline Builder::DictItemContext Builder::BaseContext::StartDict() {
        return builder_.StartDict();
    }
    inline Builder::ArrayItemContext Builder::BaseContext::StartArray() {
        return builder_.StartArray();
    }
    inline Builder& Builder::BaseContext::EndDict() {
        return builder_.EndDict();
    }
    inline Builder& Builder::BaseContext::EndArray() {
        return builder_.EndArray();
    }
    inline Node Builder::BaseContext::Build() {
        return builder_.Build();
    }

    // --- Реализация скрывающих (shadowing) методов ---
    inline Builder::DictItemContext Builder::DictKeyContext::Value(Node node) {
        return Builder::DictItemContext(builder_.Value(std::move(node)));
    }
    inline Builder::ArrayItemContext Builder::ArrayItemContext::Value(Node node) {
        return Builder::ArrayItemContext(builder_.Value(std::move(node)));
    }

} // namespace json