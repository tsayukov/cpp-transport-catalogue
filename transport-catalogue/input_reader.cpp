#include "input_reader.h"

#include <algorithm>
#include <iterator>

namespace transport_catalogue::query::input {

void Process(Query<Tag::StopCreation>& query, TransportCatalogue& transport_catalogue) {
    transport_catalogue.AddStop({ std::move(query.stop_name), query.coordinates });
}

void Process(Query<Tag::StopDistances>& query, TransportCatalogue& transport_catalogue) {
    for (const auto& [to_stop, distance] : query.distances_to_stops) {
        transport_catalogue.SetDistance(query.from_stop_name, to_stop, distance);
    }
}

void Process(Query<Tag::BusCreation>& query, TransportCatalogue& transport_catalogue) {
    Bus bus;
    bus.name = std::move(query.bus_name);

    bus.route_type = query.route_type;

    bus.stops.reserve(query.stops.size());
    std::transform(
            query.stops.cbegin(), query.stops.cend(),
            std::back_inserter(bus.stops),
            [&transport_catalogue](const auto& stop_name) {
                return transport_catalogue.FindStopBy(stop_name).value();
            });
    transport_catalogue.AddBus(std::move(bus));
}

void ProcessQueries(std::vector<query::Any>&& queries, TransportCatalogue& transport_catalogue) {
    for (auto& query : queries) {
        if (query.GetTag() == Tag::StopCreation) {
            Process(query.GetData<Tag::StopCreation>(), transport_catalogue);
        }
    }
    for (auto& query : queries) {
        if (query.GetTag() == Tag::StopCreation) {
            Process(query.GetData<Tag::StopCreation>().stop_distances_subquery, transport_catalogue);
        }
    }
    for (auto& query : queries) {
        if (query.GetTag() == Tag::BusCreation) {
            Process(query.GetData<Tag::BusCreation>(), transport_catalogue);
        }
    }
}

} // namespace transport_catalogue::query::input