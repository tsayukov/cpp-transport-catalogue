#pragma once

#include "json.h"

#include <vector>

namespace json {

class Builder {
private:
    class DictValueContext;
    class DictItemContext;
    class ArrayItemContext;

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

        DictItemContext Value(json::Value value);

        DictValueContext Key(std::string key) = delete;
        BaseContext EndDict() = delete;
        BaseContext EndArray() = delete;
        Node Build() = delete;
    };

    class DictItemContext : public BaseContext {
    public:
        /* implicit */ DictItemContext(Builder& builder) noexcept;

        DictItemContext Value(json::Value value) = delete;
        DictItemContext StartDict() = delete;
        ArrayItemContext StartArray() = delete;
        BaseContext EndArray() = delete;
        Node Build() = delete;
    };

    class ArrayItemContext : public BaseContext {
    public:
        /* implicit */ ArrayItemContext(Builder& builder) noexcept;

        DictValueContext Key(std::string key) = delete;
        BaseContext EndDict() = delete;
        Node Build() = delete;
    };

public:
    BaseContext Value(Value value);

    DictItemContext StartDict();

    ArrayItemContext StartArray();

    DictValueContext Key(std::string key) = delete;
    BaseContext EndDict() = delete;
    BaseContext EndArray() = delete;
    Node Build() = delete;

private:
    std::vector<Node> nodes_;
};

} // namespace json