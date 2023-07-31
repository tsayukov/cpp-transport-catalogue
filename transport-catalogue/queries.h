#pragma once

#include <any>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <typeindex>
#include <typeinfo>
#include <type_traits>

namespace transport_catalogue {

namespace into {

class PrintDriver {
public:
    using PrintOperation = std::function<void(std::ostream&, const void*)>;

    explicit PrintDriver(std::ostream& output) noexcept;

    virtual ~PrintDriver() = default;
    virtual void Flush() const;

    template<typename T>
    void RegisterPrintOperation(PrintOperation&& op) {
        type_to_print_operation_.emplace(std::type_index(typeid(T)), std::move(op));
    }

    template<typename T>
    void Print(const T& value) const {
        const auto type = std::type_index(typeid(std::decay_t<T>));
        PrintObject(type, &value);
    }

protected:
    std::ostream& GetOutput() const;

private:
    virtual void PrintObject(std::type_index type, const void* object) const;

    std::ostream& output_;
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

} // namespace queries

namespace from {

class Parser {
public:
    virtual ~Parser() = default;

    template<typename T>
    [[nodiscard]] T Get(std::string_view object_name) const {
        return std::any_cast<T>(GetObject(object_name));
    }

    class Result {
    public:
        void ProcessModifyQueries(queries::Handler& handler, const into::Printer& printer);
        void ProcessResponseQueries(queries::Handler& handler, const into::Printer& printer);

        void PushBack(std::unique_ptr<queries::Query>&& query_ptr);
    private:
        using Queries = std::vector<std::unique_ptr<queries::Query>>;

        Queries modify_queries_;
        Queries setup_queries_;
        Queries response_queries_;
    };

    [[nodiscard]] Result&& ReleaseResult() noexcept {
        return std::move(result_);
    }

protected:
    [[nodiscard]] Result& GetResult() noexcept;
private:
    Result result_;

    [[nodiscard]] virtual std::any GetObject(std::string_view object_name) const = 0;
};

} // namespace from

namespace queries {

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

    static const QueryFactory& GetFactory(std::type_index index);
};

} // namespace queries

} // namespace transport_catalogue