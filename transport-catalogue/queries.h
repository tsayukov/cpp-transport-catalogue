#pragma once

#include <any>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <typeindex>
#include <typeinfo>
#include <type_traits>

namespace transport_catalogue {

namespace from {

class Parser {
public:
    using QueryType = std::string_view;

    virtual ~Parser() = default;
    virtual QueryType GetQueryType() const = 0;

    template<typename T>
    T Get(std::string_view object_name) const {
        return std::any_cast<T>(GetObject(object_name));
    }

private:
    virtual std::any GetObject(std::string_view object_name) const = 0;
};

} // namespace from

namespace into {

class PrintDriver {
public:
    using PrintOperation = std::function<void(std::ostream&, const void*)>;

    std::ostream& output;

    explicit PrintDriver(std::ostream& output) noexcept;

    virtual ~PrintDriver() = default;
    virtual void Flush() const;

    template<typename T>
    void RegisterPrintOperation(PrintOperation&& op) {
        type_to_print_operation_.emplace(std::type_index(typeid(std::decay_t<T>)), std::move(op));
    }

    template<typename T>
    void Print(const T& value) const {
        const auto type = std::type_index(typeid(std::decay_t<T>));
        const auto& foo = type_to_print_operation_.at(type);
        foo(output, &value);
//        type_to_print_operation_.at(type)(output, &value);
    }

private:
    std::unordered_map<std::type_index, PrintOperation> type_to_print_operation_;
};

class Printer {
public:
    Printer(const PrintDriver& driver) noexcept;
    ~Printer();

    template<typename T>
    void Print(const T& value) const {
        driver_.Print(value);
    }

private:
    const PrintDriver& driver_;
};

} // namespace into

namespace queries {

class Handler;

class Query {
public:
    virtual ~Query() = default;
    virtual void ProcessAndPrint(Handler& handler, const into::Printer& printer) const = 0;
};

class ModifyQuery : public Query {
public:
    virtual void Process(Handler& handler) const = 0;

    void ProcessAndPrint(Handler& handler, const into::Printer&) const override;
};

template<typename Settings>
class SetupQuery : public ModifyQuery {
public:
    explicit SetupQuery(Settings settings) noexcept(std::is_nothrow_move_constructible_v<Settings>);

    const Settings& GetSettings() const;
private:
    Settings settings_;
};

class ResponseQuery : public Query {
public:
    explicit ResponseQuery(int id) noexcept;
    [[nodiscard]] int GetId() const noexcept;
private:
    int id_;
};

class NamedEntity {
public:
    explicit NamedEntity(std::string name);
    [[nodiscard]] std::string_view GetName() const noexcept;
private:
    std::string name_;
};

class QueryFactory {
public:
    virtual ~QueryFactory() = default;
    [[nodiscard]] virtual std::unique_ptr<Query> Construct(const from::Parser& parser) const = 0;

    static const QueryFactory& GetFactory(std::string_view query_name);
};

} // namespace queries

namespace from {

using Queries = std::deque<std::unique_ptr<queries::Query>>;

class ParseResult {
public:
    void ProcessModifyQueries(queries::Handler& handler, const into::Printer& printer);
    void ProcessSetupQueries(queries::Handler& handler, const into::Printer& printer);
    void ProcessResponseQueries(queries::Handler& handler, const into::Printer& printer);

    void PushBack(std::unique_ptr<queries::Query>&& query, Parser::QueryType query_type);
private:
    Queries modify_queries_;
    Queries setup_queries_;
    Queries response_queries_;
};

} // namespace from

} // namespace transport_catalogue