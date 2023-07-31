#include "queries.h"

#include "geo.h"
#include "domain.h"
#include "request_handler.h"

#include <tuple>
#include <unordered_map>
#include <vector>

namespace transport_catalogue {

namespace into {

// PrintDriver

PrintDriver::PrintDriver(std::ostream& output) noexcept
        : output_(output) {
}

void PrintDriver::Flush() const {}

std::ostream& PrintDriver::GetOutput() const {
    return output_;
}

void PrintDriver::PrintObject(std::type_index type, const void* object) const {
    type_to_print_operation_.at(type)(GetOutput(), object);
}

// Printer

Printer::Printer(const PrintDriver& driver) noexcept
        : driver_(driver) {
}

Printer::~Printer() {
    driver_.Flush();
}

} // namespace into

namespace queries {

using namespace std::string_view_literals;

// ModifyQuery

void ModifyQuery::ProcessAndPrint(Handler& handler, const into::Printer&) const {
    Process(handler);
}

// SetupQuery

template<typename Settings>
SetupQuery<Settings>::SetupQuery(Settings settings) noexcept(std::is_nothrow_move_constructible_v<Settings>)
        : settings_(std::move(settings)) {
}

template<typename Settings>
const Settings& SetupQuery<Settings>::GetSettings() const {
    return settings_;
}

// ResponseQuery

ResponseQuery::ResponseQuery(int id) noexcept
        : id_(id) {
}

int ResponseQuery::GetId() const noexcept {
    return id_;
}

// NamedEntity

NamedEntity::NamedEntity(std::string name)
        : name_(std::move(name)) {
}

std::string_view NamedEntity::GetName() const noexcept {
    return name_;
}

// Specific queries

class StopCreation : public ModifyQuery, NamedEntity {
public:
    StopCreation(std::string name, geo::Coordinates coordinates, std::unordered_map<std::string, geo::Meter> distances)
            : NamedEntity(std::move(name))
            , coordinates_(coordinates)
            , distances_(std::move(distances)) {
    }

    ~StopCreation() {
        postponed_operation();
    }

    void Process(Handler& handler) const override {
        handler.AddStop(Stop{std::string(GetName()), coordinates_});

        postponed_operation = [this, &handler] {
            for (const auto& [to_stop_name, distance] : distances_) {
                std::string_view from_stop = GetName();
                handler.SetDistanceBetweenStops(from_stop, to_stop_name, distance);
            }
        };
    }

    class Factory : public QueryFactory {
    public:
        [[nodiscard]] std::unique_ptr<Query> Construct(const from::Parser& parser) const override {
            return std::make_unique<StopCreation>(
                    parser.Get<std::string>("name"sv),
                    parser.Get<geo::Coordinates>("coordinates"sv),
                    parser.Get<std::unordered_map<std::string, geo::Meter>>("distances"sv));
        }
    };

private:
    geo::Coordinates coordinates_;
    std::unordered_map<std::string, geo::Meter> distances_;

    mutable std::function<void()> postponed_operation;
};

class BusCreation : public ModifyQuery, NamedEntity {
public:
    BusCreation(std::string name, std::vector<std::string> stop_names, Bus::RouteType route_type) noexcept
            : NamedEntity(std::move(name))
            , stop_names_(std::move(stop_names))
            , route_type_(route_type) {
    }

    ~BusCreation() {
        postponed_operation();
    }

    void Process(Handler& handler) const override {
        postponed_operation = [this, &handler] {
            handler.AddBus(std::string(GetName()), stop_names_, route_type_);
        };
    }

    class Factory : public QueryFactory {
    public:
        [[nodiscard]] std::unique_ptr<Query> Construct(const from::Parser& parser) const override {
            return std::make_unique<BusCreation>(
                    parser.Get<std::string>("name"sv),
                    parser.Get<std::vector<std::string>>("stop_names"sv),
                    parser.Get<Bus::RouteType>("route_type"sv));
        }
    };

private:
    std::vector<std::string> stop_names_;
    Bus::RouteType route_type_;

    mutable std::function<void()> postponed_operation;
};

class MapRendererSetup : public SetupQuery<renderer::Settings> {
public:
    using SetupQuery<renderer::Settings>::SetupQuery;

    void Process(Handler& handler) const override {
        handler.InitializeMapRenderer(GetSettings());
    }

    class Factory : public QueryFactory {
    public:
        [[nodiscard]] std::unique_ptr<Query> Construct(const from::Parser& parser) const override {
            return std::make_unique<MapRendererSetup>(
                    parser.Get<renderer::Settings>("renderer_settings"sv));
        }
    };
};

class TransportRouterSetup : public SetupQuery<router::Settings> {
public:
    using SetupQuery<router::Settings>::SetupQuery;

    void Process(Handler& handler) const override {
        handler.InitializeRouterSettings(GetSettings());
        handler.InitializeRouter();
    }

    class Factory : public QueryFactory {
    public:
        [[nodiscard]] std::unique_ptr<Query> Construct(const from::Parser& parser) const override {
            return std::make_unique<TransportRouterSetup>(
                    parser.Get<router::Settings>("router_settings"sv));
        }
    };
};

class SerializationSetup : public SetupQuery<serialization::Settings> {
public:
    using SetupQuery<serialization::Settings>::SetupQuery;

    void Process(Handler& handler) const override {
        handler.InitializeSerialization(GetSettings());
    }

    class Factory : public QueryFactory {
    public:
        [[nodiscard]] std::unique_ptr<Query> Construct(const from::Parser& parser) const override {
            return std::make_unique<SerializationSetup>(
                    parser.Get<serialization::Settings>("serialization_settings"sv));
        }
    };
};

class StopInfoQuery : public ResponseQuery, NamedEntity {
public:
    explicit StopInfoQuery(int id, std::string name) noexcept
            : ResponseQuery(id), NamedEntity(std::move(name)) {
    }

    void ProcessAndPrint(Handler& handler, const into::Printer& printer) const override {
        printer.Print(std::make_tuple(GetId(), GetName(), handler.GetStopInfo(GetName())));
    }

    class Factory : public QueryFactory {
    public:
        [[nodiscard]] std::unique_ptr<Query> Construct(const from::Parser& parser) const override {
            return std::make_unique<StopInfoQuery>(
                    parser.Get<int>("id"sv),
                    parser.Get<std::string>("name"sv));
        }
    };
};

class BusInfoQuery : public ResponseQuery, NamedEntity {
public:
    explicit BusInfoQuery(int id, std::string name) noexcept
            : ResponseQuery(id), NamedEntity(std::move(name)) {
    }

    void ProcessAndPrint(Handler& handler, const into::Printer& printer) const override {
        printer.Print(std::make_tuple(GetId(), GetName(), handler.GetBusInfo(GetName())));
    }

    class Factory : public QueryFactory {
    public:
        [[nodiscard]] std::unique_ptr<Query> Construct(const from::Parser& parser) const override {
            return std::make_unique<BusInfoQuery>(
                    parser.Get<int>("id"sv),
                    parser.Get<std::string>("name"sv));
        }
    };
};

class MapRenderer : public ResponseQuery {
public:
    using ResponseQuery::ResponseQuery;

    void ProcessAndPrint(Handler& handler, const into::Printer& printer) const override {
        printer.Print(std::make_tuple(GetId(), handler.RenderMap()));
    }

    class Factory : public QueryFactory {
    public:
        [[nodiscard]] std::unique_ptr<Query> Construct(const from::Parser& parser) const override {
            return std::make_unique<MapRenderer>(
                    parser.Get<int>("id"sv));
        }
    };
};

class Route : public ResponseQuery {
public:
    Route(int id, std::string from_stop, std::string to_stop)
            : ResponseQuery(id)
            , from_stop_(std::move(from_stop))
            , to_stop_(std::move(to_stop)) {
    }

    void ProcessAndPrint(Handler& handler, const into::Printer& printer) const override {
        printer.Print(std::make_tuple(GetId(), handler.GetRouteBetweenStops(from_stop_, to_stop_)));
    }

    class Factory : public QueryFactory {
    public:
        [[nodiscard]] std::unique_ptr<Query> Construct(const from::Parser& parser) const override {
            return std::make_unique<Route>(
                    parser.Get<int>("id"sv),
                    parser.Get<std::string>("from"sv),
                    parser.Get<std::string>("to"sv));
        }
    };

private:
    std::string from_stop_;
    std::string to_stop_;
};

// QueryFactory

const QueryFactory& QueryFactory::GetFactory(std::type_index index) {
    static const StopCreation::Factory stop_creation;
    static const BusCreation::Factory bus_creation;
    static const MapRendererSetup::Factory renderer_setup;
    static const TransportRouterSetup::Factory router_setup;
    static const SerializationSetup::Factory serialization_setup;
    static const StopInfoQuery::Factory stop_info;
    static const BusInfoQuery::Factory bus_info;
    static const MapRenderer::Factory renderer;
    static const Route::Factory router;

    static const std::unordered_map<std::type_index, const QueryFactory&> factories = {
            {std::type_index(typeid(Stop)), stop_creation},
            {std::type_index(typeid(Bus)), bus_creation},
            {std::type_index(typeid(renderer::Settings)), renderer_setup},
            {std::type_index(typeid(router::Settings)), router_setup},
            {std::type_index(typeid(serialization::Settings)), serialization_setup},
            {std::type_index(typeid(queries::Handler::StopInfo)), stop_info},
            {std::type_index(typeid(queries::Handler::BusInfo)), bus_info},
            {std::type_index(typeid(renderer::MapRenderer)), renderer},
            {std::type_index(typeid(router::TransportRouter)), router},
    };
    return factories.at(index);
}

} // namespace queries

namespace from {

void Parser::Result::ProcessModifyQueries(queries::Handler& handler, const into::Printer& printer) {
    for (const auto& query : modify_queries_) {
        query->ProcessAndPrint(handler, printer);
    }
    modify_queries_.clear();

    for (const auto& query : setup_queries_) {
        query->ProcessAndPrint(handler, printer);
    }

    if (response_queries_.empty()) {
        handler.Serialize();
    }
}

void Parser::Result::ProcessResponseQueries(queries::Handler& handler, const into::Printer& printer) {
    if (modify_queries_.empty()) {
        handler.Deserialize();
    }

    for (const auto& query : response_queries_) {
        query->ProcessAndPrint(handler, printer);
    }
}

void Parser::Result::PushBack(std::unique_ptr<queries::Query>&& query_ptr) {
    using namespace std::string_view_literals;

    const auto& query = *query_ptr;
    const auto& query_type = typeid(query);
    if (query_type == typeid(queries::StopCreation)
            || query_type == typeid(queries::BusCreation)) {
        modify_queries_.push_back(std::move(query_ptr));
    } else if (query_type == typeid(queries::StopInfoQuery)
            || query_type == typeid(queries::BusInfoQuery)
            || query_type == typeid(queries::MapRenderer)
            || query_type == typeid(queries::Route)) {
        response_queries_.push_back(std::move(query_ptr));
    } else if (query_type == typeid(queries::MapRendererSetup)
            || query_type == typeid(queries::TransportRouterSetup)
            || query_type == typeid(queries::SerializationSetup)) {
        setup_queries_.push_back(std::move(query_ptr));
    } else {
        using namespace std::string_literals;
        throw std::invalid_argument("Query must be either ModifyQuery or ResponseQuery"s);
    }
}

Parser::Result& Parser::GetResult() noexcept {
    return result_;
}

} // namespace from

} // namespace transport_catalogue