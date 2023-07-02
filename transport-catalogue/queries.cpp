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
        : output(output) {
}

void PrintDriver::Flush() const {}

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

    void Process(Handler& handler) const override {
        handler.AddBus(std::string(GetName()), stop_names_, route_type_);
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
        // todo setup TransportRouter
    }

    class Factory : public QueryFactory {
    public:
        [[nodiscard]] std::unique_ptr<Query> Construct(const from::Parser& parser) const override {
            return std::make_unique<TransportRouterSetup>(
                    parser.Get<router::Settings>("router_settings"sv));
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

class Router : public ResponseQuery {
public:
    using ResponseQuery::ResponseQuery;

    void ProcessAndPrint(Handler&, const into::Printer&) const override {
        // todo impl
    }

    class Factory : public QueryFactory {
    public:
        [[nodiscard]] std::unique_ptr<Query> Construct(const from::Parser& parser) const override {
            return std::make_unique<Router>(
                    parser.Get<int>("id"sv));
        }
    };
};

// QueryFactory

const QueryFactory& QueryFactory::GetFactory(std::string_view query_name) {
    static const StopCreation::Factory stop_creation;
    static const BusCreation::Factory bus_creation;
    static const MapRendererSetup::Factory renderer_setup;
    static const TransportRouterSetup::Factory router_setup;
    static const StopInfoQuery::Factory stop_info;
    static const BusInfoQuery::Factory bus_info;
    static const MapRenderer::Factory renderer;
    static const Router::Factory router;

    static const std::unordered_map<std::string_view, const QueryFactory&> factories = {
            {"stop_creation"sv,  stop_creation},
            {"bus_creation"sv,   bus_creation},
            {"renderer_setup"sv, renderer_setup},
            {"router_setup"sv,   router_setup},
            {"stop_info"sv,      stop_info},
            {"bus_info"sv,       bus_info},
            {"renderer"sv,       renderer},
            {"router"sv,         router},
    };

    return factories.at(query_name);
}

} // namespace queries

namespace from {

void ParseResult::ProcessModifyQueries(queries::Handler& handler, const into::Printer& printer) {
    for (const auto& query : modify_queries_) {
        query->ProcessAndPrint(handler, printer);
    }
    modify_queries_.clear();
}

void ParseResult::ProcessSetupQueries(queries::Handler& handler, const into::Printer& printer) {
    for (const auto& query : setup_queries_) {
        query->ProcessAndPrint(handler, printer);
    }
}

void ParseResult::ProcessResponseQueries(queries::Handler& handler, const into::Printer& printer) {
    for (const auto& query : response_queries_) {
        query->ProcessAndPrint(handler, printer);
    }
}

void ParseResult::PushBack(std::unique_ptr<queries::Query>&& query, Parser::QueryType query_type) {
    using namespace std::string_view_literals;

    if (query_type == "stop_creation"sv) {
        modify_queries_.push_front(std::move(query));
    } else if (query_type == "bus_creation"sv) {
        modify_queries_.push_back(std::move(query));
    } else if (query_type == "renderer_setup"sv || query_type == "router_setup"sv) {
        setup_queries_.push_back(std::move(query));
    } else {
        response_queries_.push_back(std::move(query));
    }
}

} // namespace from

} // namespace transport_catalogue