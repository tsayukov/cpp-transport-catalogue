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

Handler::Result::operator bool() const noexcept {
    return std::holds_alternative<std::monostate>(*this);
}

bool Handler::Result::IsIncorrectMode() const noexcept {
    return std::holds_alternative<Error>(*this) && std::get<Error>(*this) == Error::IncorrectMode;
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

void Handler::InitializeRouter() {
    router_.InitializeRouter(database_);
}

Handler::RouteResult Handler::GetRouteBetweenStops(std::string_view from, std::string_view to) {
    if (!router_.IsInitialized()) {
        router_.InitializeRouter(database_);
    }
    return router_.GetRouteBetweenStops(FindStopBy(from).value(), FindStopBy(to).value());
}

// Serialization methods adapters

void Handler::InitializeSerialization(serialization::Settings settings) {
    serializer_.Initialize(std::move(settings));
}

void Handler::Serialize() {
    serializer_.Serialize({database_, renderer_.GetSettings(), router_});
}

void Handler::Deserialize() {
    auto received_data = serializer_.Deserialize();
    database_ = std::move(received_data.database);
    if (received_data.render_settings.has_value()) {
        InitializeMapRenderer(std::move(received_data.render_settings.value()));
    }
    if (received_data.transport_router.has_value()) {
        router_.ReplaceBy(std::move(received_data.transport_router.value()));
    }
}

} // namespace transport_catalogue::queries