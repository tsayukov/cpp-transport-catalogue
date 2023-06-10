#include "request_handler.h"

#include <functional>

namespace transport_catalogue::query {

Handler::Handler(const TransportCatalogue& database, const renderer::MapRenderer& renderer) noexcept
        : database_(database)
        , renderer_(renderer) {
}

std::optional<const TransportCatalogue::BusInfo*> Handler::GetBusInfo(std::string_view bus_name) const {
    return database_.GetBusInfo(bus_name);
}

std::optional<const TransportCatalogue::StopInfo*> Handler::GetStopInfo(std::string_view stop_name) const {
    return database_.GetStopInfo(stop_name);
}

svg::Document Handler::RenderMap() const {
    auto [bus_first, bus_last] = database_.GetAllBuses();
    return renderer_.Render(bus_first, bus_last);
}

} // namespace transport_catalogue::query