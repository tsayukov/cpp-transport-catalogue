#include "json_builder.h"

#include <stdexcept>
#include <variant>

namespace json {

using namespace std::string_literals;

// BaseContext

Builder::BaseContext::BaseContext(Builder& builder) noexcept
        : builder_(builder) {
}

Builder& Builder::BaseContext::GetBuilder() noexcept {
    return builder_;
}

Builder::DictValueContext Builder::BaseContext::Key(std::string key) {
    if (!GetNodes().back().IsMap()) {
        throw std::logic_error("There is no dictionary for key insertion"s);
    }

    GetNodes().emplace_back(std::move(key));
    return GetBuilder();
}

Builder::ArrayItemContext Builder::BaseContext::Value(json::Value value) {
    if (!GetNodes().back().IsArray()) {
        throw std::logic_error("There is no array for value insertion"s);
    }

    std::get<Array>(GetNodes().back().GetValue()).emplace_back(std::move(value));
    return GetBuilder();
}

Builder::DictItemContext Builder::BaseContext::StartDict() {
    if (!GetNodes().back().IsArray()) {
        throw std::logic_error("There is no array for dictionary insertion"s);
    }

    GetNodes().emplace_back(Map{});
    return GetBuilder();
}

Builder::BaseContext Builder::BaseContext::EndDict() {
    if (!GetNodes().back().IsMap()) {
        throw std::logic_error("There is no dictionary to close"s);
    }

    if (GetNodes().size() == 1) {
        return GetBuilder();
    }

    Node dict = std::move(GetNodes().back());
    GetNodes().pop_back();

    if (GetNodes().back().IsArray()) {
        return Value(std::move(dict.GetValue()));
    } else if (GetNodes().back().IsString()) {
        return DictValueContext(builder_).Value(std::move(dict.GetValue()));
    }
    throw std::logic_error("Unreachable case"s);
}

Builder::ArrayItemContext Builder::BaseContext::StartArray() {
    if (!GetNodes().back().IsArray()) {
        throw std::logic_error("There is no array for array insertion"s);
    }

    GetNodes().emplace_back(Array{});
    return GetBuilder();
}

Builder::BaseContext Builder::BaseContext::EndArray() {
    if (!GetNodes().back().IsArray()) {
        throw std::logic_error("There is no array to close"s);
    }

    if (GetNodes().size() == 1) {
        return GetBuilder();
    }

    Node array = std::move(GetNodes().back());
    GetNodes().pop_back();

    if (GetNodes().back().IsArray()) {
        return Value(std::move(array.GetValue()));
    } else if (GetNodes().back().IsString()) {
        return DictValueContext(builder_).Value(std::move(array.GetValue()));
    }
    throw std::logic_error("Unreachable case"s);
}

Node Builder::BaseContext::Build() {
    if (GetNodes().size() > 1) {
        throw std::logic_error("JSON is not completed yet"s);
    }

    Node node = std::move(GetNodes().back());
    GetNodes().pop_back();
    return node;
}

std::vector<Node>& Builder::BaseContext::GetNodes() noexcept {
    return builder_.nodes_;
}

// DictValueContext

Builder::DictValueContext::DictValueContext(Builder& builder) noexcept
        : BaseContext(builder) {
}

Builder::DictItemContext Builder::DictValueContext::Value(json::Value value) {
    std::string key = std::move(std::get<std::string>(GetNodes().back().GetValue()));
    GetNodes().pop_back();

    Node& dict = GetNodes().back();
    std::get<Map>(dict.GetValue()).emplace(std::move(key), std::move(value));
    return GetBuilder();
}

// DictItemContext

Builder::DictItemContext::DictItemContext(Builder& builder) noexcept
        : BaseContext(builder) {
}

// ArrayItemContext

Builder::ArrayItemContext::ArrayItemContext(Builder& builder) noexcept
        : BaseContext(builder) {
}

// Builder

Builder::BaseContext Builder::Value(json::Value value) {
    nodes_.emplace_back(std::move(value));
    return *this;
}

Builder::DictItemContext Builder::StartDict() {
    nodes_.emplace_back(Map{});
    return *this;
}

Builder::ArrayItemContext Builder::StartArray() {
    nodes_.emplace_back(Array{});
    return *this;
}

} // namespace json
