#include "transport_catalogue.h"

#include <algorithm>
#include <unordered_set>
#include <stdexcept>
#include <cassert>

namespace transport_catalogue {

void TransportCatalogue::AddStop(Stop stop) {
    stops_.push_back(std::move(stop));
    const Stop* stop_ptr = &stops_.back();
    stop_indices_.emplace(stop_ptr->name, stop_ptr);
}

void TransportCatalogue::AddBus(Bus bus) {
    buses_.push_back(std::move(bus));
    const Bus* bus_ptr = &buses_.back();
    bus_indices_.emplace(bus_ptr->name, bus_ptr);

    for (const Stop* stop : bus_ptr->stops) {
        stop_statistics_[stop].buses.emplace_back(bus_ptr);
    }
}

void TransportCatalogue::SetDistance(std::string_view stop_name_from, std::string_view stop_name_to,
                                     geo::Meter distance) {
    auto iter_from = stop_indices_.find(stop_name_from);
    auto iter_to = stop_indices_.find(stop_name_to);
    if (iter_from == stop_indices_.end() || iter_to == stop_indices_.end()) {
        return;
    }

    const Stop* stop_from_ptr = iter_from->second;
    const Stop* stop_to_ptr = iter_to->second;
    distances_.emplace(std::make_pair(stop_from_ptr, stop_to_ptr), distance);
}

std::optional<geo::Meter> TransportCatalogue::GetDistance(std::string_view stop_name_from,
                                                     std::string_view stop_name_to) const {
    auto iter_from = stop_indices_.find(stop_name_from);
    auto iter_to = stop_indices_.find(stop_name_to);
    if (iter_from == stop_indices_.end() || iter_to == stop_indices_.end()) {
        return std::nullopt;
    }

    const Stop* stop_from_ptr = iter_from->second;
    const Stop* stop_to_ptr = iter_to->second;
    return GetDistanceBetween(stop_from_ptr, stop_to_ptr);
}

[[nodiscard]] std::optional<const Bus*> TransportCatalogue::FindBusBy(std::string_view bus_name) const {
    if (auto iter = bus_indices_.find(bus_name); iter == bus_indices_.end()) {
        return std::nullopt;
    } else {
        const Bus* bus = iter->second;
        return bus;
    }
}

[[nodiscard]] std::optional<const Stop*> TransportCatalogue::FindStopBy(std::string_view stop_name) const {
    if (auto iter = stop_indices_.find(stop_name); iter == stop_indices_.end()) {
        return std::nullopt;
    } else {
        const Stop* stop = iter->second;
        return stop;
    }
}

// Statistics

TransportCatalogue::StopInfo TransportCatalogue::GetStopInfo(std::string_view stop_name) const {
    const auto stop = FindStopBy(stop_name);
    if (!stop.has_value()) {
        return { stop_name, std::nullopt };
    }

    auto iter_stop_stat = stop_statistics_.find(stop.value());
    if (iter_stop_stat == stop_statistics_.end()) {
        return { stop.value()->name, std::vector<const Bus*>() };
    }

    auto& stats = iter_stop_stat->second;
    auto& buses = stats.buses;
    if (!iter_stop_stat->second.is_buses_unique_and_ordered) {
        std::sort(buses.begin(), buses.end(), [](const Bus* lhs, const Bus* rhs) noexcept {
                return lhs->name < rhs->name;
        });
        auto begin_to_remove = std::unique(buses.begin(), buses.end(), [](const Bus* lhs, const Bus* rhs) noexcept {
                return lhs->name == rhs->name;
        });
        buses.erase(begin_to_remove, buses.end());
        stats.is_buses_unique_and_ordered = true;
    }
    return { stop.value()->name, std::make_optional(buses) };
}

[[nodiscard]] TransportCatalogue::BusInfo TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    const auto bus = FindBusBy(bus_name);
    if (!bus.has_value()) {
        return { bus_name, std::nullopt };
    }

    auto iter_bus_stat = bus_statistics_.find(bus.value());
    const auto stats = (iter_bus_stat == bus_statistics_.end())
                     ? ComputeBusStatistics(bus.value())
                     : iter_bus_stat->second;
    return { bus.value()->name, stats };
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

const Bus::Statistics& TransportCatalogue::ComputeBusStatistics(const Bus* bus_ptr) const {
    assert(bus_ptr != nullptr);

    constexpr unsigned int stops_buses_factor = 2;

    auto [iter, _] = bus_statistics_.emplace(bus_ptr, Bus::Statistics());
    const Bus& bus = *bus_ptr;
    Bus::Statistics& stats = iter->second;

    std::unordered_set<const Stop*> unique_stops;
    unique_stops.reserve(stats.stops_count / stops_buses_factor);

    details::KahanFloatingPointAddition double_sum_route_length;
    details::KahanFloatingPointAddition double_sum_geo_length;

    for (std::size_t i = 0; i < bus.stops.size() - 1; ++i) {
        unique_stops.emplace(bus.stops[i]);

        auto distance = GetDistanceBetween(bus.stops[i], bus.stops[i + 1]);
        stats.route_length = double_sum_route_length(distance);

        auto geo_distance = GetGeoDistanceBetween(bus.stops[i], bus.stops[i + 1]);
        stats.geo_length = double_sum_geo_length(geo_distance);
    }

    stats.stops_count = bus.stops.size();
    stats.unique_stops_count = unique_stops.size();
    return stats;
}

// Distance

std::size_t TransportCatalogue::StopPtrsHasher::operator()(std::pair<const Stop*, const Stop*> stops) const noexcept {
    static std::hash<const Stop*> stop_hash;
    constexpr unsigned int prime_num = 37;
    return stop_hash(stops.first) + prime_num * stop_hash(stops.second);
}

[[nodiscard]] geo::Meter TransportCatalogue::GetDistanceBetween(const Stop* from, const Stop* to) const {
    auto iter = distances_.find({from, to});
    if (iter != distances_.end()) {
        return iter->second;
    }

    auto distance = geo::Meter(0);
    iter = distances_.find({to, from});
    assert(iter != distances_.end());

    distance = iter->second;
    distances_.emplace(std::make_pair(from, to), distance);

    return distance;
}

[[nodiscard]] geo::Meter TransportCatalogue::GetGeoDistanceBetween(const Stop* from, const Stop* to) const noexcept {
    return geo::ComputeDistance(from->coordinates, to->coordinates);
}

} // namespace transport_catalogue