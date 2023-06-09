#include "transport_catalogue.h"

#include <algorithm>
#include <cassert>
#include <unordered_set>

namespace transport_catalogue {

void TransportCatalogue::AddStop(const Stop& stop) {
    Stop stop_copy = stop;
    AddStop(std::move(stop_copy));
}

void TransportCatalogue::AddStop(Stop&& stop) {
    stops_.push_back(std::move(stop));
    StopPtr stop_ptr = &stops_.back();
    stop_indices_.emplace(stop_ptr->name, stop_ptr);
}

void TransportCatalogue::AddBus(const Bus& bus) {
    Bus bus_copy = bus;
    AddBus(std::move(bus_copy));
}

void TransportCatalogue::AddBus(Bus&& bus) {
    buses_.push_back(std::move(bus));
    BusPtr bus_ptr = &buses_.back();
    bus_indices_.emplace(bus_ptr->name, bus_ptr);

    for (StopPtr stop_ptr : bus_ptr->stops) {
        auto iter = stop_statistics_.find(stop_ptr);
        if (iter != stop_statistics_.end()) {
            iter->second.buses.emplace_back(bus_ptr);
        } else {
            StopInfo stop_info{};
            stop_info.buses.emplace_back(bus_ptr);
            stop_statistics_.emplace(stop_ptr, std::move(stop_info));
        }
    }
}

void TransportCatalogue::SetDistance(std::string_view stop_name_from, std::string_view stop_name_to,
                                     geo::Meter distance) {
    auto iter_from = stop_indices_.find(stop_name_from);
    auto iter_to = stop_indices_.find(stop_name_to);
    if (iter_from == stop_indices_.end() || iter_to == stop_indices_.end()) {
        return;
    }

    StopPtr stop_from_ptr = iter_from->second;
    StopPtr stop_to_ptr = iter_to->second;
    distances_.emplace(std::make_pair(stop_from_ptr, stop_to_ptr), distance);
}

std::optional<geo::Meter> TransportCatalogue::GetDistance(std::string_view stop_name_from,
                                                          std::string_view stop_name_to) const {
    auto iter_from = stop_indices_.find(stop_name_from);
    auto iter_to = stop_indices_.find(stop_name_to);
    if (iter_from == stop_indices_.end() || iter_to == stop_indices_.end()) {
        return std::nullopt;
    }

    const Stop& stop_from = *iter_from->second;
    const Stop& stop_to = *iter_to->second;
    return GetDistance(stop_from, stop_to);
}

std::optional<BusPtr> TransportCatalogue::FindBusBy(std::string_view bus_name) const {
    if (auto iter = bus_indices_.find(bus_name); iter == bus_indices_.end()) {
        return std::nullopt;
    } else {
        BusPtr bus = iter->second;
        return bus;
    }
}

std::optional<StopPtr> TransportCatalogue::FindStopBy(std::string_view stop_name) const {
    if (auto iter = stop_indices_.find(stop_name); iter == stop_indices_.end()) {
        return std::nullopt;
    } else {
        StopPtr stop = iter->second;
        return stop;
    }
}

TransportCatalogue::StopConstIters TransportCatalogue::GetAllStops() const noexcept {
    return { stops_.cbegin(), stops_.cend() };
}

TransportCatalogue::BusConstIters TransportCatalogue::GetAllBuses() const noexcept {
    return { buses_.cbegin(), buses_.cend() };
}

// Statistics

TransportCatalogue::StopInfo::StopInfo(std::vector<BusPtr> buses)
        : buses(std::move(buses)) {
}

std::optional<const TransportCatalogue::StopInfo*> TransportCatalogue::GetStopInfo(std::string_view stop_name) const {
    const auto stop = FindStopBy(stop_name);
    if (!stop.has_value()) {
        return std::nullopt;
    }

    auto iter_stop_stat = stop_statistics_.find(stop.value());
    if (iter_stop_stat == stop_statistics_.end()) {
        const auto& stop_info = stop_statistics_.emplace(stop.value(), StopInfo{}).first->second;
        return &stop_info;
    }

    StopInfo& stop_info = iter_stop_stat->second;
    auto& buses = stop_info.buses;
    if (!stop_info.is_buses_unique_and_ordered) {
        std::sort(buses.begin(), buses.end(), [](const Bus* lhs, const Bus* rhs) noexcept {
                return lhs->name < rhs->name;
        });
        auto begin_to_remove = std::unique(buses.begin(), buses.end(), [](const Bus* lhs, const Bus* rhs) noexcept {
                return lhs->name == rhs->name;
        });
        buses.erase(begin_to_remove, buses.end());
        stop_info.is_buses_unique_and_ordered = true;
    }
    return &stop_info;
}

std::optional<const TransportCatalogue::BusInfo*> TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    const auto bus = FindBusBy(bus_name);
    if (!bus.has_value()) {
        return std::nullopt;
    }

    auto iter_bus_stat = bus_statistics_.find(bus.value());
    const BusInfo& bus_info = (iter_bus_stat == bus_statistics_.end())
                            ? ComputeBusStatistics(*bus.value())
                            : iter_bus_stat->second;
    return &bus_info;
}

namespace details {

/// Kahan summation algorithm
/// source: https://en.wikipedia.org/wiki/Kahan_summation_algorithm
struct KahanFloatingPointAddition {
    double acc = 0.0;
    double compensation = 0.0;

    [[nodiscard]] double operator()(double value) noexcept {
        double y = value - compensation;
        double t = acc + y;
        compensation = (t - acc) - y;
        acc = t;
        return acc;
    }
};

}

const TransportCatalogue::BusInfo& TransportCatalogue::ComputeBusStatistics(const Bus& bus) const {
    constexpr unsigned int stops_buses_factor = 2;

    auto [iter, _] = bus_statistics_.emplace(&bus, BusInfo{});

    BusInfo& bus_info = iter->second;

    std::unordered_set<const Stop*> unique_stops;
    unique_stops.reserve(bus_info.stops_count / stops_buses_factor);

    details::KahanFloatingPointAddition double_sum_route_length;
    details::KahanFloatingPointAddition double_sum_geo_length;

    for (std::size_t i = 0; i < bus.stops.size() - 1; ++i) {
        unique_stops.emplace(bus.stops[i]);

        auto distance = GetDistance(*bus.stops[i], *bus.stops[i + 1]);
        bus_info.route_length = double_sum_route_length(distance);

        auto geo_distance = GetGeoDistance(*bus.stops[i], *bus.stops[i + 1]);
        bus_info.geo_length = double_sum_geo_length(geo_distance);
    }

    bus_info.stops_count = bus.stops.size();
    bus_info.unique_stops_count = unique_stops.size();
    return bus_info;
}

// Distance

std::size_t TransportCatalogue::StopPtrPairHasher::operator()(std::pair<StopPtr, StopPtr> stops) const noexcept {
    static std::hash<StopPtr> stop_hash;
    constexpr unsigned int prime_num = 37;
    return stop_hash(stops.first) + prime_num * stop_hash(stops.second);
}

geo::Meter TransportCatalogue::GetDistance(const Stop& from, const Stop& to) const {
    auto iter = distances_.find({&from, &to});
    if (iter != distances_.end()) {
        return iter->second;
    }

    auto distance = geo::Meter(0);
    iter = distances_.find({&to, &from});
    assert(iter != distances_.end());

    distance = iter->second;
    distances_.emplace(std::make_pair(&from, &to), distance);

    return distance;
}

} // namespace transport_catalogue