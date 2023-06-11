#include "json_reader.h"

#include <algorithm>
#include <iterator>
#include <sstream>

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
            array.push_back(AsJsonNode(id, bus_info));
        } else if (query.GetTag() == Tag::StopInfo) {
            auto stop_info = handler.GetStopInfo(query.GetData<Tag::StopInfo>().stop_name);
            array.push_back(AsJsonNode(id, stop_info));
        } else if (query.GetTag() == Tag::RenderMap) {
            array.push_back(AsJsonNode(id, handler.RenderMap()));
        }
    }
    json::Print(json::Document(json::Node(std::move(array))), output);
}

json::Node AsJsonNode(int id, std::optional<const TransportCatalogue::StopInfo*> stop_info) {
    if (!stop_info.has_value()) {
        return json::Builder{}
                .StartDict()
                    .Key("request_id"s).Value(id)
                    .Key("error_message"s).Value("not found"s)
                .EndDict()
                .Build();
    }

    const auto& buses = stop_info.value()->buses;
    return json::Builder{}
            .StartDict()
                .Key("request_id"s).Value(id)
                .Key("buses"s)
                    .Value(std::invoke([&buses] {
                        json::Array bus_array;
                        bus_array.reserve(buses.size());
                        std::transform(
                                buses.cbegin(), buses.cend(),
                                std::back_inserter(bus_array),
                                [](BusPtr bus_ptr) {
                                    return bus_ptr->name;
                                });
                        return bus_array;
                    }))
            .EndDict()
            .Build();
}

json::Node AsJsonNode(int id, std::optional<const TransportCatalogue::BusInfo*> bus_info) {
    if (!bus_info.has_value()) {
        return json::Builder{}
                .StartDict()
                    .Key("request_id"s).Value(id)
                    .Key("error_message"s).Value("not found"s)
                .EndDict()
                .Build();
    }

    auto bus_stats_ptr = bus_info.value();
    return json::Builder{}
            .StartDict()
                .Key("request_id"s).Value(id)
                .Key("stop_count"s).Value(static_cast<int>(bus_stats_ptr->stops_count))
                .Key("unique_stop_count"s).Value(static_cast<int>(bus_stats_ptr->unique_stops_count))
                .Key("route_length"s).Value(bus_stats_ptr->route_length)
                .Key("curvature"s).Value(bus_stats_ptr->route_length / bus_stats_ptr->geo_length)
            .EndDict()
            .Build();
}

json::Node AsJsonNode(int id, const svg::Document& document) {
    std::ostringstream str_output;
    document.Render(str_output);

    return json::Builder{}
        .StartDict()
            .Key("request_id"s).Value(id)
            .Key("map"s).Value(str_output.str())
        .EndDict()
        .Build();
}

} // namespace transport_catalogue::query