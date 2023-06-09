#include "json_reader.h"

#include <algorithm>
#include <iterator>

namespace transport_catalogue::query {

using namespace std::string_literals;

void ProcessBaseQueries(reader::ResultType<reader::From::Json>& queries, TransportCatalogue& transport_catalogue) {
    input::ProcessQueries(std::move(queries.base_queries_), transport_catalogue);
}

void ProcessRenderSettings(reader::ResultType<reader::From::Json>& queries, renderer::MapRenderer& renderer) {
    renderer.Initialize(std::move(queries.render_settings_.settings));
}

void ProcessStatQueries(reader::ResultType<reader::From::Json>& queries, std::ostream& output, const Handler& handler) {
    json::Array array;
    for (auto& [id, query] : queries.stat_queries_) {
        if (query.GetTag() == Tag::BusInfo) {
            auto bus_info = handler.GetBusInfo(query.GetData<Tag::BusInfo>().bus_name);
            array.push_back(AsJsonNode<std::optional<const TransportCatalogue::BusInfo*>>(id, bus_info));
        } else if (query.GetTag() == Tag::StopInfo) {
            auto stop_info = handler.GetStopInfo(query.GetData<Tag::StopInfo>().stop_name);
            array.push_back(AsJsonNode<std::optional<const TransportCatalogue::StopInfo*>>(id, stop_info));
        }
    }
    json::Print(json::Document(json::Node(std::move(array))), output);
}

template<>
[[nodiscard]] json::Node AsJsonNode<std::optional<const TransportCatalogue::StopInfo*>>(
        int id, std::optional<const TransportCatalogue::StopInfo*> stop_info) {
    json::Map map;
    map.emplace("request_id"s, json::Node(id));
    if (!stop_info.has_value()) {
        map.emplace("error_message"s, json::Node("not found"s));
        return map;
    }

    const auto& buses = stop_info.value()->buses;
    json::Array bus_array;
    bus_array.reserve(buses.size());
    std::transform(
            buses.cbegin(), buses.cend(),
            std::back_inserter(bus_array),
            [](BusPtr bus_ptr) {
                return bus_ptr->name;
            });
    map.emplace("buses"s, json::Node(std::move(bus_array)));

    return map;
}

template<>
[[nodiscard]] json::Node AsJsonNode<std::optional<const TransportCatalogue::BusInfo*>>(
        int id, std::optional<const TransportCatalogue::BusInfo*> bus_info) {
    json::Map map;
    map.emplace("request_id"s, json::Node(id));
    if (!bus_info.has_value()) {
        map.emplace("error_message"s, json::Node("not found"s));
        return map;
    }

    auto bus_stats_ptr = bus_info.value();
    const int stop_count = static_cast<int>(bus_stats_ptr->stops_count);
    map.emplace("stop_count"s, json::Node(stop_count));

    const int unique_stops_count = static_cast<int>(bus_stats_ptr->unique_stops_count);
    map.emplace("unique_stop_count"s, json::Node(unique_stops_count));

    const double route_length = bus_stats_ptr->route_length;
    map.emplace("route_length"s, json::Node(route_length));

    const double curvature = bus_stats_ptr->route_length / bus_stats_ptr->geo_length;
    map.emplace("curvature"s, json::Node(curvature));

    return map;
}

} // namespace transport_catalogue::query