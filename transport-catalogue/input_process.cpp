#include "input_process.h"

#include <algorithm>
#include <iterator>

namespace transport_catalogue::query::input {

template<>
void Process<Tag::StopCreation>(Query<Tag::StopCreation>& query, TransportCatalogue* transport_catalogue_ptr) {
    assert(transport_catalogue_ptr != nullptr);
    assert(query.latitude >= -90.0 && query.latitude <= 90.0);
    assert(query.longitude >= -180.0 && query.longitude <= 180.0);

    transport_catalogue_ptr->AddStop({ std::move(query.stop_name), { query.latitude, query.longitude } });
}

template<>
void Process<Tag::StopDistances>(Query<Tag::StopDistances>& query, TransportCatalogue* transport_catalogue_ptr) {
    for (const auto& [to_stop, distance] : query.distances_to_stops) {
        transport_catalogue_ptr->SetDistance(query.from_stop_name, to_stop, distance);
    }
}

template<>
void Process<Tag::BusCreation>(Query<Tag::BusCreation>& query, TransportCatalogue* transport_catalogue_ptr) {
    assert(transport_catalogue_ptr != nullptr);
    assert(query.stops.size() > 1);

    Bus bus;
    bus.name = std::move(query.bus_name);

    using RouteView = Query<Tag::BusCreation>::RouteView;
    const auto num_stops = (query.route_view == RouteView::Full) ? query.stops.size() : query.stops.size() * 2 - 1;
    bus.stops.reserve(num_stops);
    std::transform(
            query.stops.cbegin(), query.stops.cend(),
            std::back_inserter(bus.stops),
            [transport_catalogue_ptr](const auto& stop_name) {
                return transport_catalogue_ptr->FindStopBy(stop_name).value();
            });
    if (query.route_view == RouteView::Half) {
        std::copy(bus.stops.crbegin() + 1, bus.stops.crend(), std::back_inserter(bus.stops));
    }
    transport_catalogue_ptr->AddBus(std::move(bus));
}

void BulkProcess(std::vector<query::Any>& queries, TransportCatalogue* transport_catalogue_ptr) {
    assert(transport_catalogue_ptr != nullptr);

    for (auto& query : queries) {
        if (query.GetTag() == Tag::StopCreation) {
            Process(query.GetData<Tag::StopCreation>(), transport_catalogue_ptr);
        }
    }
    for (auto& query : queries) {
        if (query.GetTag() == Tag::StopCreation) {
            Process(query.GetData<Tag::StopCreation>().stop_distances_subquery, transport_catalogue_ptr);
        }
    }
    for (auto& query : queries) {
        if (query.GetTag() == Tag::BusCreation) {
            Process(query.GetData<Tag::BusCreation>(), transport_catalogue_ptr);
        }
    }
}

} // namespace transport_catalogue::query::input