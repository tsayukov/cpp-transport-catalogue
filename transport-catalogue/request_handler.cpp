#include "request_handler.h"

#include <functional>

namespace transport_catalogue::queries {

Handler::Handler(TransportCatalogue& database,
                 renderer::MapRenderer& renderer,
                 router::TransportRouter& router) noexcept
        : database_(database)
        , renderer_(renderer)
        , router_(router) {
}

// Transport Catalogue methods adapters

void Handler::AddStop(Stop stop) {
    database_.AddStop(std::move(stop));
}

void Handler::AddBus(Bus bus) {
    database_.AddBus(std::move(bus));
}

void Handler::SetDistanceBetweenStops(std::string_view from, std::string_view to, geo::Meter distance) {
    database_.SetDistanceBetweenStops(from, to, distance);
}

[[nodiscard]] std::optional<geo::Meter> Handler::GetDistanceBetweenStops(std::string_view from,
                                                                         std::string_view to) const {
    return database_.GetDistanceBetweenStops(from, to);
}

[[nodiscard]] std::optional<StopPtr> Handler::FindStopBy(std::string_view stop_name) const {
    return database_.FindStopBy(stop_name);
}

[[nodiscard]] std::optional<BusPtr> Handler::FindBusBy(std::string_view bus_name) const {
    return database_.FindBusBy(bus_name);
}

[[nodiscard]] TransportCatalogue::StopRange Handler::GetAllStops() const noexcept {
    return database_.GetAllStops();
}

[[nodiscard]] TransportCatalogue::BusRange Handler::GetAllBuses() const noexcept {
    return database_.GetAllBuses();
}

std::optional<const Handler::BusInfo*> Handler::GetBusInfo(std::string_view bus_name) const {
    return database_.GetBusInfo(bus_name);
}

std::optional<const Handler::StopInfo*> Handler::GetStopInfo(std::string_view stop_name) const {
    return database_.GetStopInfo(stop_name);
}

// Map Renderer methods adapters

void Handler::InitializeMapRenderer(renderer::Settings settings) {
    renderer_.Initialize(std::move(settings));
}

svg::Document Handler::RenderMap() const {
    const auto bus_range = database_.GetAllBuses();
    return renderer_.Render(bus_range.begin(), bus_range.end());
}

// Transport Route methods adapters

void Handler::InitializeRouterSettings(router::Settings settings) {
    router_.Initialize(settings);
}

Handler::RouteResult Handler::GetRouteBetweenStops(std::string_view from, std::string_view to) {
    if (!router_.IsInitialized()) {
        router_.InitializeRouter(database_);
    }
    return router_.GetRouteBetweenStops(FindStopBy(from).value(), FindStopBy(to).value());
}

} // namespace transport_catalogue::queries