#include "json_builder.h"

#include <stdexcept>
#include <variant>

namespace json {

using namespace std::string_literals;

// Builder

BaseContext Builder::Value(json::Value value) {
    nodes_.emplace_back(std::move(value));
    return *this;
}

DictItemContext Builder::StartDict() {
    nodes_.emplace_back(Map{});
    return *this;
}

ArrayItemContext Builder::StartArray() {
    nodes_.emplace_back(Array{});
    return *this;
}

// BaseContext

BaseContext::BaseContext(Builder& builder) noexcept
        : builder_(builder) {
}

Builder& BaseContext::GetBuilder() noexcept {
    return builder_;
}

DictValueContext BaseContext::Key(std::string key) {
    if (!GetNodes().back().IsMap()) {
        throw std::logic_error("There is no dictionary for key insertion"s);
    }

    GetNodes().emplace_back(std::move(key));
    return GetBuilder();
}

ArrayItemContext BaseContext::Value(json::Value value) {
    if (!GetNodes().back().IsArray()) {
        throw std::logic_error("There is no array for value insertion"s);
    }

    std::get<Array>(GetNodes().back().GetValue()).emplace_back(std::move(value));
    return GetBuilder();
}

DictItemContext BaseContext::StartDict() {
    if (!GetNodes().back().IsArray()) {
        throw std::logic_error("There is no array for dictionary insertion"s);
    }

    GetNodes().emplace_back(Map{});
    return GetBuilder();
}

BaseContext BaseContext::EndDict() {
    if (!GetNodes().back().IsMap()) {
        throw std::logic_error("There is no dictionary to close"s);
    }

    if (GetNodes().size() == 1) {
        return GetBuilder();
    }

    Node dict = std::move(GetNodes().back());
    GetNodes().pop_back();

    if (GetNodes().back().IsArray()) {
        std::get<Array>(GetNodes().back().GetValue()).emplace_back(std::move(dict));
    } else if (GetNodes().back().IsString()) {
        std::string key = std::move(std::get<std::string>(GetNodes().back().GetValue()));
        GetNodes().pop_back();
        std::get<Map>(GetNodes().back().GetValue()).emplace(std::move(key), std::move(dict));
    }
    return GetBuilder();
}

ArrayItemContext BaseContext::StartArray() {
    if (!GetNodes().back().IsArray()) {
        throw std::logic_error("There is no array for array insertion"s);
    }

    GetNodes().emplace_back(Array{});
    return GetBuilder();
}

BaseContext BaseContext::EndArray() {
    if (!GetNodes().back().IsArray()) {
        throw std::logic_error("There is no array to close"s);
    }

    if (GetNodes().size() == 1) {
        return GetBuilder();
    }

    Node array = std::move(GetNodes().back());
    GetNodes().pop_back();

    if (GetNodes().back().IsArray()) {
        std::get<Array>(GetNodes().back().GetValue()).emplace_back(std::move(array));
    } else if (GetNodes().back().IsString()) {
        std::string key = std::move(std::get<std::string>(GetNodes().back().GetValue()));
        GetNodes().pop_back();
        std::get<Map>(GetNodes().back().GetValue()).emplace(std::move(key), std::move(array));
    }
    return GetBuilder();
}

Node BaseContext::Build() {
    if (GetNodes().size() > 1) {
        throw std::logic_error("JSON is not completed yet"s);
    }

    Node node = std::move(GetNodes().back());
    GetNodes().pop_back();
    return node;
}

std::vector<Node>& BaseContext::GetNodes() noexcept {
    return builder_.nodes_;
}

// DictValueContext

DictValueContext::DictValueContext(Builder& builder) noexcept
        : BaseContext(builder) {
}

DictItemContext DictValueContext::Value(json::Value value) {
    std::string key = std::move(std::get<std::string>(GetNodes().back().GetValue()));
    GetNodes().pop_back();

    Node& dict = GetNodes().back();
    std::get<Map>(dict.GetValue()).emplace(std::move(key), std::move(value));
    return GetBuilder();
}

DictItemContext DictValueContext::StartDict() {
    GetNodes().emplace_back(Map{});
    return GetBuilder();
}

ArrayItemContext DictValueContext::StartArray() {
    GetNodes().emplace_back(Array{});
    return GetBuilder();
}

// DictItemContext

DictItemContext::DictItemContext(Builder& builder) noexcept
        : BaseContext(builder) {
}

DictValueContext DictItemContext::Key(std::string key) {
    GetNodes().emplace_back(std::move(key));
    return GetBuilder();
}

BaseContext DictItemContext::EndDict() {
    if (GetNodes().size() == 1) {
        return GetBuilder();
    }

    Node dict = std::move(GetNodes().back());
    GetNodes().pop_back();

    if (GetNodes().back().IsArray()) {
        std::get<Array>(GetNodes().back().GetValue()).emplace_back(std::move(dict));
    } else if (GetNodes().back().IsString()) {
        std::string key = std::move(std::get<std::string>(GetNodes().back().GetValue()));
        GetNodes().pop_back();
        std::get<Map>(GetNodes().back().GetValue()).emplace(std::move(key), std::move(dict));
    }
    return GetBuilder();
}

// ArrayItemContext

ArrayItemContext::ArrayItemContext(Builder& builder) noexcept
        : BaseContext(builder) {
}

ArrayItemContext ArrayItemContext::Value(json::Value value) {
    std::get<Array>(GetNodes().back().GetValue()).emplace_back(std::move(value));
    return GetBuilder();
}

DictItemContext ArrayItemContext::StartDict() {
    GetNodes().emplace_back(Map{});
    return GetBuilder();
}

ArrayItemContext ArrayItemContext::StartArray() {
    GetNodes().emplace_back(Array{});
    return GetBuilder();
}

BaseContext ArrayItemContext::EndArray() {
    if (GetNodes().size() == 1) {
        return GetBuilder();
    }

    Node array = std::move(GetNodes().back());
    GetNodes().pop_back();

    if (GetNodes().back().IsArray()) {
        std::get<Array>(GetNodes().back().GetValue()).emplace_back(std::move(array));
    } else if (GetNodes().back().IsString()) {
        std::string key = std::move(std::get<std::string>(GetNodes().back().GetValue()));
        GetNodes().pop_back();
        std::get<Map>(GetNodes().back().GetValue()).emplace(std::move(key), std::move(array));
    }
    return GetBuilder();
}

} // namespace json
