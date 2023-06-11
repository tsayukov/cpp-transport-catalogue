#include "json.h"

#include <stdexcept>

namespace json {

namespace {

using namespace std::string_literals;
using namespace std::string_view_literals;

[[nodiscard]] Node LoadNode(std::istream& input);

[[nodiscard]] Node LoadNumber(std::istream& input) {
    std::string parsed_num;

    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }

    if (input.peek() == '0') {
        read_char();
    } else {
        read_digits();
    }

    bool is_int = true;

    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            try {
                return Node(std::stoi(parsed_num));
            } catch (...) {
                // pass
            }
        }
        return Node(std::stod(parsed_num));
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to a number"s);
    }
}

[[nodiscard]] Node LoadString(std::istream& input) {
    std::string str;

    auto check_input = [&input] {
        if (!input) {
            throw ParsingError("Failed to read a character from the stream"s);
        }
    };

    auto iter = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();

    while (true) {
        check_input();

        if (iter == end) {
            throw ParsingError("String ended before a closed double quote"s);
        }

        const char ch = *iter;
        if (ch == '"') {
            ++iter;
            check_input();
            break;
        } else if (ch == '\\') {
            ++iter;
            check_input();
            if (iter == end) {
                throw ParsingError("String ended before a closed double quote"s);
            }
            const char escaped_char = *iter;
            switch (escaped_char) {
                case 'n':
                    str.push_back('\n');
                    break;
                case 't':
                    str.push_back('\t');
                    break;
                case 'r':
                    str.push_back('\r');
                    break;
                case '"':
                    str.push_back('"');
                    break;
                case '\\':
                    str.push_back('\\');
                    break;
                default:
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            throw ParsingError("Unexpected end of line inside a string literal"s);
        } else {
            str.push_back(ch);
        }

        ++iter;
    }

    return Node(std::move(str));
}

bool TryReadNextChar(std::istream& input, char& ch) {
    if (!(input >> ch)) {
        throw ParsingError("Failed to read a character from the stream"s);
    }
    return true;
}

bool TrySkipWsAndReadNextChar(std::istream& input, char& ch) {
    if (!(input >> std::ws >> ch)) {
        throw ParsingError("Failed to read a character from the stream"s);
    }
    return true;
}

[[nodiscard]] Node LoadArray(std::istream& input) {
    Array result;

    char ch;
    TrySkipWsAndReadNextChar(input, ch);

    if (ch == ',') {
        throw ParsingError("Comma right after an open bracket"s);
    }
    if (ch == ']') {
        return Node(std::move(result));
    }

    while (true) {
        if (!input.putback(ch)) {
            throw ParsingError("Failed to put a character back into the stream"s);
        }
        result.push_back(LoadNode(input));

        TrySkipWsAndReadNextChar(input, ch);
        if (ch == ']') {
            break;
        } else if (ch != ',') {
            throw ParsingError("Pairs of a key and value must be separated by a comma"s);
        }

        TrySkipWsAndReadNextChar(input, ch);
        if (ch == ']') {
            throw ParsingError("Trailing comma is not allowed"s);
        }
    }

    return Node(std::move(result));
}

[[nodiscard]] Node LoadDict(std::istream& input) {
    Map result;

    char ch;
    TrySkipWsAndReadNextChar(input, ch);

    if (ch == ',') {
        throw ParsingError("Comma right after an open curly brace"s);
    }
    if (ch == '}') {
        return Node(std::move(result));
    }

    while (true) {
        std::string key;
        if (ch == '"') {
            key = LoadString(input).AsString();
        } else {
            throw ParsingError("Dictionary key must start after a double quote"s);
        }

        TrySkipWsAndReadNextChar(input, ch);
        if (ch != ':') {
            throw ParsingError("Dictionary key and value must be separated by a colon"s);
        }

        result.emplace(std::move(key), LoadNode(input));

        TrySkipWsAndReadNextChar(input, ch);
        if (ch == '}') {
            break;
        } else if (ch != ',') {
            throw ParsingError("Pairs of a key and value must be separated by a comma"s);
        }

        TrySkipWsAndReadNextChar(input, ch);
        if (ch == '}') {
            throw ParsingError("Trailing comma is not allowed"s);
        }
    }

    return Node(std::move(result));
}

[[nodiscard]] Node LoadNode(std::istream& input) {
    char ch;

    TrySkipWsAndReadNextChar(input, ch);
    if (ch == '[') {
        return LoadArray(input);
    } else if (ch == '{') {
        return LoadDict(input);
    } else if (ch == '"') {
        return LoadString(input);
    } else if (/*      check true literal     */ ch == 't') {
        if (       TryReadNextChar(input, ch) && ch == 'r'
                && TryReadNextChar(input, ch) && ch == 'u'
                && TryReadNextChar(input, ch) && ch == 'e') {
            return Node(true);
        } else {
            throw ParsingError("Unrecognized literal; maybe it was meant `true`"s);
        }
    } else if (/*      check false literal    */ ch == 'f') {
        if (       TryReadNextChar(input, ch) && ch == 'a'
                && TryReadNextChar(input, ch) && ch == 'l'
                && TryReadNextChar(input, ch) && ch == 's'
                && TryReadNextChar(input, ch) && ch == 'e') {
            return Node(false);
        } else {
            throw ParsingError("Unrecognized literal; maybe it was meant `false`"s);
        }
    } else if (/*      check null literal     */ ch == 'n') {
        if (       TryReadNextChar(input, ch) && ch == 'u'
                && TryReadNextChar(input, ch) && ch == 'l'
                && TryReadNextChar(input, ch) && ch == 'l') {
            return Node();
        } else {
            throw ParsingError("Unrecognized literal; maybe it was meant `null`"s);
        }
    } else {
        if (!input.putback(ch)) {
            throw ParsingError("Failed to put a character back into the stream"s);
        }
        return LoadNumber(input);
    }
}

} // namespace

// Node

Node::Node(Value value)
        : Value(std::move(value)) {
}

bool Node::IsNull() const noexcept {
    return std::holds_alternative<std::nullptr_t>(*this);
}

bool Node::IsBool() const noexcept {
    return std::holds_alternative<bool>(*this);
}

bool Node::IsInt() const noexcept {
    return std::holds_alternative<int>(*this);
}

bool Node::IsDouble() const noexcept {
    return std::holds_alternative<double>(*this) || IsInt();
}

bool Node::IsPureDouble() const noexcept {
    return std::holds_alternative<double>(*this);
}

bool Node::IsString() const noexcept {
    return std::holds_alternative<std::string>(*this);
}

bool Node::IsArray() const noexcept {
    return std::holds_alternative<Array>(*this);
}

bool Node::IsMap() const noexcept {
    return std::holds_alternative<Map>(*this);
}

bool Node::AsBool() const {
    return TryGetAs<bool>();
}

int Node::AsInt() const {
    return TryGetAs<int>();
}

double Node::AsDouble() const {
    try {
        return TryGetAs<double>();
    } catch (...) {
        return TryGetAs<int>();
    }
}

const std::string& Node::AsString() const {
    return TryGetAs<const std::string&>();
}

const Array& Node::AsArray() const {
    return TryGetAs<const Array&>();
}

const Map& Node::AsMap() const {
    return TryGetAs<const Map&>();
}

bool Node::operator==(const Node& rhs) const noexcept {
    return *static_cast<const Value*>(this) == rhs;
}

bool Node::operator!=(const Node& rhs) const noexcept {
    return !(*this == rhs);
}

Value& Node::GetValue() noexcept {
    return *static_cast<Value*>(this);
}

const Value& Node::GetValue() const noexcept {
    return *static_cast<const Value*>(this);
}

// Document

Document::Document(Node root)
    : root_(std::move(root)) {
}

Node& Document::GetRoot() {
    return root_;
}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator==(const Document& rhs) const {
    return root_ == rhs.root_;
}

bool Document::operator!=(const Document& rhs) const {
    return !(*this == rhs);
}

Document Load(std::istream& input) {
    return Document(LoadNode(input));
}

// Print

PrintContext::PrintContext(std::ostream& output, unsigned int indent_step, unsigned int indent) noexcept
        : output(output)
        , indent_step(indent_step)
        , indent(indent) {
}

void PrintContext::PrintIndent() const {
    for (unsigned int i = 0; i < indent; ++i) {
        output.put(' ');
    }
}

PrintContext PrintContext::Indented() const noexcept {
    return PrintContext(output, indent_step, indent_step + indent);
}

void PrintValue::operator()(std::nullptr_t) {
    context.output << "null"sv;
}

void PrintValue::operator()(bool value) {
    context.output << std::boolalpha << value;
}

void PrintValue::operator()(int value) {
    context.output << value;
}

void PrintValue::operator()(double value) {
    context.output << value;
}

void PrintValue::operator()(const std::string& value) {
    auto& output = context.output;
    std::ostreambuf_iterator output_buf(output);

    output_buf = '\"';
    for (const char ch : value) {
        switch (ch) {
            case '\n':
                output_buf = '\\';
                output_buf = 'n';
                break;
            case '\r':
                output_buf = '\\';
                output_buf = 'r';
                break;
            case '\"':
                output_buf = '\\';
                output_buf = '\"';
                break;
            case '\\':
                output_buf = '\\';
                output_buf = '\\';
                break;
            default:
                output_buf = ch;
        }
    }
    output_buf = '\"';
}

void PrintValue::operator()(const Array& array) {
    auto& output = context.output;

    output << "["sv;
    if (array.empty()) {
        output << "]"sv;
        return;
    }

    const PrintContext inner_context = context.Indented();
    output << "\n"sv;
    inner_context.PrintIndent();
    std::visit(PrintValue{ inner_context }, array.front().GetValue());
    for (auto iter = ++array.begin(), last = array.end(); iter != last; ++iter) {
        output << ",\n"sv;
        inner_context.PrintIndent();
        std::visit(PrintValue{ inner_context }, iter->GetValue());
    }
    if (!array.empty()) {
        output << "\n"sv;
    }
    context.PrintIndent();
    output << "]"sv;
}

void PrintValue::operator()(const Map& dict) {
    auto& output = context.output;

    output << "{"sv;
    if (dict.empty()) {
        output << "}"sv;
        return;
    }

    auto iter = dict.begin();
    const PrintContext inner_context = context.Indented();
    {
        output << "\n"sv;
        inner_context.PrintIndent();
        const auto& [key, node] = *iter;
        output << "\""sv << key << "\": "sv;
        std::visit(PrintValue{ inner_context }, node.GetValue());
    }
    ++iter;
    for (auto last = dict.end(); iter != last; ++iter) {
        output << ",\n"sv;
        inner_context.PrintIndent();
        const auto& [key, node] = *iter;
        output << "\""sv << key << "\": "sv;
        std::visit(PrintValue{ inner_context }, node.GetValue());
    }
    if (!dict.empty()) {
        output << "\n"sv;
    }
    context.PrintIndent();
    output << "}"sv;
}

void Print(const Document& document, std::ostream& output) {
    const PrintContext context(output, 4);
    std::visit(PrintValue{ context }, document.GetRoot().GetValue());
}

}  // namespace json