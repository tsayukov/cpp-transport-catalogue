#pragma once

#include "json.h"

#include <vector>

namespace json {

class BaseContext;
class DictValueContext;
class DictItemContext;
class ArrayItemContext;

class Builder {
public:
    DictValueContext Key(std::string key) = delete;

    BaseContext Value(Value value);

    DictItemContext StartDict();
    BaseContext EndDict() = delete;

    ArrayItemContext StartArray();
    BaseContext EndArray() = delete;

    Node Build() = delete;

private:
    friend BaseContext;
    std::vector<Node> nodes_;
};

class BaseContext {
public:
    /* implicit */ BaseContext(Builder& builder) noexcept;

    DictValueContext Key(std::string key);

    ArrayItemContext Value(Value value);

    DictItemContext StartDict();
    BaseContext EndDict();

    ArrayItemContext StartArray();
    BaseContext EndArray();

    Node Build();

protected:
    Builder& GetBuilder() noexcept;
    std::vector<Node>& GetNodes() noexcept;
private:
    Builder& builder_;
};

class DictValueContext : public BaseContext {
public:
    /* implicit */ DictValueContext(Builder& builder) noexcept;

    DictValueContext Key(std::string key) = delete;

    DictItemContext Value(json::Value value);

    DictItemContext StartDict();
    BaseContext EndDict() = delete;

    ArrayItemContext StartArray();
    BaseContext EndArray() = delete;

    Node Build() = delete;
};

class DictItemContext : public BaseContext {
public:
    /* implicit */ DictItemContext(Builder& builder) noexcept;

    DictValueContext Key(std::string key);

    DictItemContext Value(json::Value value) = delete;

    DictItemContext StartDict() = delete;
    BaseContext EndDict();

    ArrayItemContext StartArray() = delete;
    BaseContext EndArray() = delete;

    Node Build() = delete;
};

class ArrayItemContext : public BaseContext {
public:
    /* implicit */ ArrayItemContext(Builder& builder) noexcept;

    DictValueContext Key(std::string key) = delete;

    ArrayItemContext Value(json::Value value);

    DictItemContext StartDict();
    BaseContext EndDict() = delete;

    ArrayItemContext StartArray();
    BaseContext EndArray();

    Node Build() = delete;
};

} // namespace json