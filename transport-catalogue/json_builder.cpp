#include "json_builder.h"
#include <utility>
#include <stdexcept>

namespace json {
    Builder::Builder() {
        nodes_stack_.push_back(&root_);
    }

    Node* Builder::GetCurrentNode() {
        if (nodes_stack_.empty()) {
            throw std::logic_error("Attempt to modify a fully built JSON");
        }
        return nodes_stack_.back();
    }

    Builder::DictKeyContext Builder::Key(std::string key) {
        Node* current = GetCurrentNode();

        if (!current->IsDict()) {
            throw std::logic_error("Key can only be called unside a Dict");
        }

        if (key_) {
            throw std::logic_error("Key was already set: " + *key_);
        }

        key_ = std::move(key);
        return DictKeyContext(*this);
    }

    Builder& Builder::Value(Node node) {
        Node* current = GetCurrentNode();
        Node::Value value = node.GetValue();

        if (current->IsDict()) {
            if (!key_) {
                throw std::logic_error("Value without key in Dict");
            }
            auto& dict = current->AsDict();
            dict.emplace(std::move(*key_), Node(std::move(value)));
            key_.reset();
        }
        else if (current->IsArray()) {
            auto& arr = current->AsArray();
            arr.emplace_back(Node(std::move(value)));
        }
        else if (current == &root_ && !root_initialized_) {
            root_ = Node(std::move(value));
            nodes_stack_.pop_back();
            root_initialized_ = true;
        }
        else {
            throw std::logic_error("Value is invalid state");
        }

        return *this;
    }

    Builder::DictItemContext Builder::StartDict() {
        Node* current = GetCurrentNode();

        if (current->IsDict()) {
            if (!key_) {
                throw std::logic_error("StartDict without key in Dict");
            }
            auto& dict = current->AsDict();
            auto [it, inserted] = dict.emplace(std::move(*key_), Node(Dict{}));
            key_.reset();
            nodes_stack_.push_back(&it->second);
        }
        else if (current->IsArray()) {
            auto& arr = current->AsArray();
            arr.emplace_back(Node(Dict{}));
            nodes_stack_.push_back(&arr.back());
        }
        else  if (current == &root_ && !root_initialized_) {
            root_ = Node(Dict{});
            root_initialized_ = true;
        }
        else {
            throw std::logic_error("StartDict in invalid state");
        }

        return DictItemContext(*this);
    }

    Builder::ArrayItemContext Builder::StartArray() {
        Node* current = GetCurrentNode();

        if (current->IsDict()) {
            if (!key_) {
                throw std::logic_error("StartArrray without key in Dict");
            }
            auto& dict = current->AsDict();
            auto [it, inserted] = dict.emplace(std::move(*key_), Node(Array{}));
            key_.reset();
            nodes_stack_.push_back(&it->second);
        }
        else if (current->IsArray()) {
            auto& arr = current->AsArray();
            arr.emplace_back(Node(Array{}));
            nodes_stack_.push_back(&arr.back());
        }
        else if (current == &root_ && !root_initialized_) {
            root_ = Node(Array{});
            root_initialized_ = true;
        }
        else {
            throw std::logic_error("StartArray in invalid state");
        }
        return ArrayItemContext(*this);
    }

    Builder& Builder::EndDict() {
        Node* current = GetCurrentNode();
        if (!current->IsDict()) {
            throw std::logic_error("EndDict called outside of Dict");
        }
        if (key_) {
            throw std::logic_error("EndDict called with a pending key");
        }
        nodes_stack_.pop_back();
        return *this;
    }

    Builder& Builder::EndArray() {
        Node* current = GetCurrentNode();

        if (!current->IsArray()) {
            throw std::logic_error("EndArray called outside of Array");
        }
        nodes_stack_.pop_back();
        return *this;
    }

    Node Builder::Build() {
        if (!root_initialized_ || !nodes_stack_.empty()) {
            throw std::logic_error("JSON is not fully built");
        }
        return std::move(root_);
    }
}