/// \file
/// Parsing JSON from input and writing into output

#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

// Node

class Node;

using Map = std::map<std::string, Node>;
using Array = std::vector<Node>;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

using Value = std::variant<std::nullptr_t, bool, int, double, std::string, Array, Map>;

class Node final : private Value {
public:
    using Value::variant;

    [[nodiscard]] bool IsNull() const noexcept;
    [[nodiscard]] bool IsBool() const noexcept;
    [[nodiscard]] bool IsInt() const noexcept;
    [[nodiscard]] bool IsDouble() const noexcept;
    [[nodiscard]] bool IsPureDouble() const noexcept;
    [[nodiscard]] bool IsString() const noexcept;
    [[nodiscard]] bool IsArray() const noexcept;
    [[nodiscard]] bool IsMap() const noexcept;

    [[nodiscard]] bool AsBool() const;
    [[nodiscard]] int AsInt() const;
    [[nodiscard]] double AsDouble() const;
    [[nodiscard]] const std::string& AsString() const;
    [[nodiscard]] const Array& AsArray() const;
    [[nodiscard]] const Map& AsMap() const;

    [[nodiscard]] bool operator==(const Node& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const Node& rhs) const noexcept;

    [[nodiscard]] const Value& GetValue() noexcept;
    [[nodiscard]] const Value& GetValue() const noexcept;

private:
    template<typename R>
    [[nodiscard]] R TryGetAs() const {
        if (auto value_ptr = std::get_if<std::decay_t<R>>(this)) {
            return *value_ptr;
        }
        using namespace std::string_literals;
        throw std::logic_error("Node doesn't contain such type"s);
    }
};

// Document

class Document {
public:
    explicit Document(Node root);

    [[nodiscard]] Node& GetRoot();
    [[nodiscard]] const Node& GetRoot() const;

    [[nodiscard]] bool operator==(const Document& rhs) const;
    [[nodiscard]] bool operator!=(const Document& rhs) const;

private:
    Node root_;
};

Document Load(std::istream& input);

// Print

struct PrintContext {
    std::ostream& output;
    unsigned int indent_step;
    unsigned int indent;

    explicit PrintContext(std::ostream& output, unsigned int indent_step = 0, unsigned int indent = 0) noexcept;

    void PrintIndent() const;

    [[nodiscard]] PrintContext Indented() const noexcept;
};

struct PrintValue {
    PrintContext context;

    void operator()(std::nullptr_t);
    void operator()(bool value);
    void operator()(int value);
    void operator()(double value);
    void operator()(const std::string& value);
    void operator()(const Array& array);
    void operator()(const Map& dict);
};

void Print(const Document& document, std::ostream& output);

}  // namespace json