#include "request_handler.h"

#include <unordered_set>

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
    auto [all_stops_begin, all_stops_end] = database_.GetAllStops();
    std::unordered_set<StopPtr> active_stops;
    active_stops.reserve(std::distance(all_stops_begin, all_stops_end));
    for (; all_stops_begin != all_stops_end; ++all_stops_begin) {
        if (auto stop_info = GetStopInfo(all_stops_begin->name);
                stop_info.has_value() && !(stop_info.value()->buses.empty())) {
            active_stops.emplace(&*all_stops_begin);
        }
    }

    auto [all_buses_begin, all_buses_end] = database_.GetAllBuses();
    std::vector<BusPtr> buses;
    buses.reserve(std::distance(all_buses_begin, all_buses_end));
    std::transform(
            all_buses_begin, all_buses_end,
            std::back_inserter(buses),
            [](const Bus& bus) noexcept {
                return &bus;
            });
    std::sort(
            buses.begin(), buses.end(),
            [](BusPtr lhs, BusPtr rhs) noexcept {
                return lhs->name < rhs->name;
            });

    return renderer_.Render(std::move(active_stops), std::move(buses));
}

} // namespace transport_catalogue::query