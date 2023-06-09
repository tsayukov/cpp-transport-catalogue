#include "stat_reader.h"

#include <iomanip>

namespace transport_catalogue::query::output {

using namespace std::string_literals;

std::ostream& operator<<(std::ostream& output, std::optional<const TransportCatalogue::BusInfo*> bus_info) {
    if (!bus_info.has_value()) {
        return output << "not found"s;
    }
    auto bus_stats_ptr = bus_info.value();
    output << bus_stats_ptr->stops_count << " stops on route, "s
           << bus_stats_ptr->unique_stops_count << " unique stops, "s
           << std::setprecision(6)
           << bus_stats_ptr->route_length << " route length, "s
           << bus_stats_ptr->route_length / bus_stats_ptr->geo_length << " curvature"s;
    return output;
}

std::ostream& operator<<(std::ostream& output, std::optional<const TransportCatalogue::StopInfo*> stop_info) {
    if (!stop_info.has_value()) {
        return output << "not found"s;
    }
    const auto& buses = stop_info.value()->buses;
    if (buses.empty()) {
        return output << "no buses"s;
    }
    output << "buses"s;
    for (BusPtr bus_ptr : buses) {
        output << " "s << bus_ptr->name;
    }
    return output;
}

std::ostream& PrintBusInfo(std::ostream& output,
                           std::string_view bus_name, std::optional<const TransportCatalogue::BusInfo*> bus_info) {
    output << "Bus "s << bus_name << ": "s;
    return output << bus_info << '\n';
}

std::ostream& PrintStopInfo(std::ostream& output,
                            std::string_view stop_name, std::optional<const TransportCatalogue::StopInfo*> stop_info) {
    output << "Stop "s << stop_name << ": "s;
    return output << stop_info << '\n';
}

void ProcessQueries(reader::ResultType<reader::From::Cli>&& queries, std::ostream& output, const Handler& handler) {
    for (auto& query : queries) {
        if (query.GetTag() == Tag::BusInfo) {
            const std::string_view bus_name = query.GetData<Tag::BusInfo>().bus_name;
            auto bus_info = handler.GetBusInfo(bus_name);
            PrintBusInfo(output, bus_name, bus_info);
        } else if (query.GetTag() == Tag::StopInfo) {
            const std::string_view stop_name = query.GetData<Tag::StopInfo>().stop_name;
            auto stop_info = handler.GetStopInfo(stop_name);
            PrintStopInfo(output, stop_name, stop_info);
        }
    }
}

} // transport_catalogue::query::output