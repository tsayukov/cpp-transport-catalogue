#include "output_process.h"

namespace transport_catalogue::query::output {

template<>
TransportCatalogue::BusInfo ProcessOutput<Tag::BusInfo, TransportCatalogue::BusInfo>(
        Query<Tag::BusInfo>& query, const TransportCatalogue* transport_catalogue_ptr) {
    assert(transport_catalogue_ptr != nullptr);

    return transport_catalogue_ptr->GetBusInfo(query.bus_name);
}

template<>
TransportCatalogue::StopInfo ProcessOutput<Tag::StopInfo, TransportCatalogue::StopInfo>(
        Query<Tag::StopInfo>& query, const TransportCatalogue* transport_catalogue_ptr) {
    assert(transport_catalogue_ptr != nullptr);

    return transport_catalogue_ptr->GetStopInfo(query.stop_name);
}

std::ostream& operator<<(std::ostream& output, const TransportCatalogue::BusInfo& bus_info) {
    using namespace std::string_literals;

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
    using namespace std::string_literals;

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

void BulkProcessOutput(std::vector<query::Any>& queries, std::ostream& output,
                       const TransportCatalogue* transport_catalogue_ptr) {
    for (auto& query : queries) {
        if (query.GetTag() == Tag::BusInfo) {
            const auto& bus_info = ProcessOutput<Tag::BusInfo, TransportCatalogue::BusInfo>(
                    query.GetData<Tag::BusInfo>(), transport_catalogue_ptr);
            output << bus_info << '\n';
        } else if (query.GetTag() == Tag::StopInfo) {
            const auto& stop_info = ProcessOutput<Tag::StopInfo, TransportCatalogue::StopInfo>(
                    query.GetData<Tag::StopInfo>(), transport_catalogue_ptr);
            output << stop_info << '\n';
        }
    }
}

} // transport_catalogue::query::output