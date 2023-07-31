/// \file
/// Facade for processing queries to TransportCatalogue database

#pragma once

#include "geo.h"
#include "domain.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "serialization.h"
#include "reader.h"
#include "input_reader.h"
#include "stat_reader.h"
#include "json_reader.h"

#include <iostream>
#include <optional>
#include <string_view>
#include <variant>

namespace transport_catalogue::queries {

class Handler final {
public:
    enum class Error {
        IncorrectMode,
    };

    class Result : private std::variant<std::monostate, Error> {
    public:
        using std::variant<std::monostate, Error>::variant;

        [[nodiscard]] /* implicit */ operator bool() const noexcept;

        [[nodiscard]] bool IsIncorrectMode() const noexcept;
    };

    explicit Handler(TransportCatalogue& database,
                     renderer::MapRenderer& renderer,
                     router::TransportRouter& router) noexcept;

    // Queries process methods adapters

    template<typename From>
    void ProcessQueries(From from);

    template<typename From, typename Into>
    void ProcessQueries(From from, Into into);

    template<typename From, typename Into>
    Handler::Result ProcessQueries(std::string_view mode, From from, Into into);

    // Transport Catalogue methods adapters

    void AddStop(Stop stop);
    void AddBus(Bus bus);

    template<typename StopContainer>
    void AddBus(std::string name, const StopContainer& stop_names, Bus::RouteType route_type);

    void SetDistanceBetweenStops(std::string_view from, std::string_view to, geo::Meter distance);
    [[nodiscard]] std::optional<geo::Meter> GetDistanceBetweenStops(std::string_view from, std::string_view to) const;

    [[nodiscard]] std::optional<StopPtr> FindStopBy(std::string_view stop_name) const;
    [[nodiscard]] std::optional<BusPtr> FindBusBy(std::string_view bus_name) const;

    [[nodiscard]] TransportCatalogue::StopRange GetAllStops() const noexcept;
    [[nodiscard]] TransportCatalogue::BusRange GetAllBuses() const noexcept;

    using BusInfo = TransportCatalogue::BusInfo;
    using StopInfo = TransportCatalogue::StopInfo;

    [[nodiscard]] std::optional<const BusInfo*> GetBusInfo(std::string_view bus_name) const;
    [[nodiscard]] std::optional<const StopInfo*> GetStopInfo(std::string_view stop_name) const;

    // Map Renderer methods adapters

    void InitializeMapRenderer(renderer::Settings settings);

    [[nodiscard]] svg::Document RenderMap() const;

    // Transport Route methods adapters

    void InitializeRouterSettings(router::Settings settings);
    void InitializeRouter();

    using RouteResult = router::TransportRouter::Result;

    [[nodiscard]] RouteResult GetRouteBetweenStops(std::string_view from, std::string_view to);

    // Serialization methods adapters

    void InitializeSerialization(serialization::Settings settings);

    void Serialize();
    void Deserialize();

private:
    TransportCatalogue& database_;
    renderer::MapRenderer& renderer_;
    router::TransportRouter& router_;

    serialization::Serializer serializer_;
};

template<typename StopContainer>
void Handler::AddBus(std::string name, const StopContainer& stop_names, Bus::RouteType route_type) {
    database_.AddBus(std::move(name), stop_names, route_type);
}

template<typename From>
void Handler::ProcessQueries(From from) {
    from::ProcessQueries(from, *this);
}

template<typename From, typename Into>
void Handler::ProcessQueries(From from, Into into) {
    into::ProcessQueries(ReadQueries(from), *this, into);
}

template<typename From, typename Into>
Handler::Result Handler::ProcessQueries(std::string_view mode, From from, Into into) {
    using namespace std::string_view_literals;

    if (mode == "make_base"sv) {
        ProcessQueries(from);
    } else if (mode == "process_requests"sv) {
        ProcessQueries(from, into);
    } else {
        return Error::IncorrectMode;
    }
    return {};
}

} // namespace transport_catalogue::queries