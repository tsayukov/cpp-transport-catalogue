#include "stat_reader.h"

#include <iomanip>

namespace transport_catalogue::query::output {

using namespace std::string_literals;

std::ostream& operator<<(std::ostream& output, const TransportCatalogue::BusInfo& bus_info) {
    output << "Bus "s << bus_info.bus_name << ": "s;
    if (!bus_info.statistics.has_value()) {
        return output << "not found"s;
    }
    output << bus_info.statistics->stops_count << " stops on route, "s
           << bus_info.statistics->unique_stops_count << " unique stops, "s
           << std::setprecision(6)
           << bus_info.statistics->route_length << " route length, "s
           << bus_info.statistics->route_length / bus_info.statistics->geo_length << " curvature"s;
    return output;
}

std::ostream& operator<<(std::ostream& output, const TransportCatalogue::StopInfo& stop_info) {
    output << "Stop "s << stop_info.stop_name << ": "s;
    if (!stop_info.buses.has_value()) {
        return output << "not found"s;
    }
    const auto& buses = stop_info.buses.value();
    if (buses.empty()) {
        return output << "no buses"s;
    }
    output << "buses"s;
    for (const Bus* bus_ptr : buses) {
        output << " "s << bus_ptr->name;
    }
    return output;
}

std::ostream& PrintBusInfo(std::ostream& output, const TransportCatalogue::BusInfo& bus_info) {
    return output << bus_info << '\n';
}

std::ostream& PrintStopInfo(std::ostream& output, const TransportCatalogue::StopInfo& stop_info) {
    return output << stop_info << '\n';
}

void ProcessQueries(std::vector<query::Any>&& queries, std::ostream& output,
                    const TransportCatalogue& transport_catalogue) {
    for (auto& query : queries) {
        if (query.GetTag() == Tag::BusInfo) {
            const auto& bus_info = transport_catalogue.GetBusInfo(query.GetData<Tag::BusInfo>().bus_name);
            PrintBusInfo(output, bus_info);
        } else if (query.GetTag() == Tag::StopInfo) {
            const auto& stop_info = transport_catalogue.GetStopInfo(query.GetData<Tag::StopInfo>().stop_name);
            PrintStopInfo(output, stop_info);
        }
    }
}

} // transport_catalogue::query::output