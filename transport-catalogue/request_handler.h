/// \file
/// Facade for processing queries to TransportCatalogue database

#pragma once

#include "geo.h"
#include "domain.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "input_reader.h"
#include "stat_reader.h"
#include "json_reader.h"

#include <iostream>
#include <optional>
#include <string_view>

namespace transport_catalogue::queries {

class Handler final {
public:
    explicit Handler(TransportCatalogue& database,
                     renderer::MapRenderer& renderer,
                     router::TransportRouter& router) noexcept;

    // Queries process methods adapters

    template<typename From, typename Into>
    void ProcessQueries(From from, std::istream& input, Into into, std::ostream& output);

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

    // Transport Router methods adapters

    // todo impl

private:
    TransportCatalogue& database_;
    renderer::MapRenderer& renderer_;
    router::TransportRouter& router_;
};

template<typename StopContainer>
void Handler::AddBus(std::string name, const StopContainer& stop_names, Bus::RouteType route_type) {
    database_.AddBus(std::move(name), stop_names, route_type);
}

template<typename From, typename Into>
void Handler::ProcessQueries(From from, std::istream& input, Into into, std::ostream& output) {
    into::ProcessQueries(ReadQueries(from, input), *this, into, output);
}

} // namespace transport_catalogue::queries