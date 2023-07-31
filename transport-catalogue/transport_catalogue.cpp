#include "transport_catalogue.h"

#include "kahan_algorithm.h"

#include <unordered_set>

namespace transport_catalogue {

void TransportCatalogue::AddStop(Stop stop) {
    stops_.push_back(std::move(stop));
    StopPtr stop_ptr = &stops_.back();
    stop_indices_.emplace(stop_ptr->name, stop_ptr);
}

void TransportCatalogue::AddBus(Bus bus) {
    buses_.push_back(std::move(bus));
    BusPtr bus_ptr = &buses_.back();
    bus_indices_.emplace(bus_ptr->name, bus_ptr);

    for (StopPtr stop_ptr : bus_ptr->stops) {
        StopInfoStorage* stop_info_storage_ptr = nullptr;
        if (auto stop_info_iter = stop_infos_.find(stop_ptr); stop_info_iter != stop_infos_.end()) {
            stop_info_storage_ptr = &stop_info_iter->second;
        } else {
            auto [iter, _] = stop_infos_.emplace(stop_ptr, StopInfoStorage{});
            stop_info_storage_ptr = &iter->second;
        }
        stop_info_storage_ptr->stop_info.buses.emplace_back(bus_ptr);
        stop_info_storage_ptr->is_buses_sorted_and_unique = false;
    }
}

void TransportCatalogue::SetDistanceBetweenStops(std::string_view from, std::string_view to, geo::Meter distance) {
    auto stop_ptr_from = FindStopBy(from);
    auto stop_ptr_to = FindStopBy(to);
    if (!stop_ptr_from.has_value() || !stop_ptr_to.has_value()) {
        using namespace std::string_literals;
        throw std::domain_error(
                "Error occurs during setting a distance between stops: one or both of the stops does not exist"s);
    }
    distances_.emplace(std::make_pair(stop_ptr_from.value(), stop_ptr_to.value()), distance);
}

std::optional<geo::Meter> TransportCatalogue::GetDistanceBetweenStops(std::string_view from, std::string_view to) const {
    auto stop_ptr_from = FindStopBy(from);
    auto stop_ptr_to = FindStopBy(to);
    if (!stop_ptr_from.has_value() || !stop_ptr_to.has_value()) {
        return std::nullopt;
    }
    return GetDistance(*stop_ptr_from.value(), *stop_ptr_to.value());
}

std::optional<BusPtr> TransportCatalogue::FindBusBy(std::string_view bus_name) const {
    if (auto iter = bus_indices_.find(bus_name); iter != bus_indices_.end()) {
        BusPtr bus_ptr = iter->second;
        return bus_ptr;
    }
    return std::nullopt;
}

std::optional<StopPtr> TransportCatalogue::FindStopBy(std::string_view stop_name) const {
    if (auto iter = stop_indices_.find(stop_name); iter != stop_indices_.end()) {
        StopPtr stop_ptr = iter->second;
        return stop_ptr;
    }
    return std::nullopt;
}

TransportCatalogue::StopRange TransportCatalogue::GetAllStops() const noexcept {
    return ranges::AsConstRange(stops_);
}

TransportCatalogue::BusRange TransportCatalogue::GetAllBuses() const noexcept {
    return ranges::AsConstRange(buses_);
}

TransportCatalogue::DistanceRange TransportCatalogue::GetDistances() const noexcept {
    return ranges::AsConstRange(distances_);
}

// Statistics

std::optional<const TransportCatalogue::StopInfo*> TransportCatalogue::GetStopInfo(std::string_view stop_name) const {
    const auto stop = FindStopBy(stop_name);
    if (!stop.has_value()) {
        return std::nullopt;
    }

    if (auto stop_info_iter = stop_infos_.find(stop.value()); stop_info_iter != stop_infos_.end()) {
        auto& stop_info_storage = stop_info_iter->second;
        return &PrepareStopInfo(stop_info_storage);
    }

    auto [stop_info_iter, _] = stop_infos_.emplace(stop.value(), StopInfoStorage{});
    const auto& stop_info_storage = stop_info_iter->second;
    return &stop_info_storage.stop_info;
}

std::optional<const TransportCatalogue::BusInfo*> TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    const auto bus = FindBusBy(bus_name);
    if (!bus.has_value()) {
        return std::nullopt;
    }

    if (auto bus_stat_iter = bus_infos_.find(bus.value()); bus_stat_iter != bus_infos_.end()) {
        return &bus_stat_iter->second;
    }
    return &ComputeBusInfo(*bus.value());
}

const TransportCatalogue::StopInfo& TransportCatalogue::PrepareStopInfo(StopInfoStorage& stop_info_storage) {
    auto& buses = stop_info_storage.stop_info.buses;
    if (!stop_info_storage.is_buses_sorted_and_unique) {
        std::sort(buses.begin(), buses.end(), [](BusPtr lhs, BusPtr rhs) noexcept {
            return lhs->name < rhs->name;
        });
        auto begin_to_remove = std::unique(buses.begin(), buses.end(), [](BusPtr lhs, BusPtr rhs) noexcept {
            return lhs->name == rhs->name;
        });
        buses.erase(begin_to_remove, buses.end());
        stop_info_storage.is_buses_sorted_and_unique = true;
    }
    return stop_info_storage.stop_info;
}

const TransportCatalogue::BusInfo& TransportCatalogue::ComputeBusInfo(const Bus& bus) const {
    auto [bus_info_iter, _] = bus_infos_.emplace(&bus, BusInfo{});
    BusInfo& bus_info = bus_info_iter->second;

    const auto& stops = bus.stops;
    std::unordered_set<const Stop*> unique_stops;
    unique_stops.reserve(stops.size());
    for (StopPtr stop_ptr : stops) {
        unique_stops.emplace(stop_ptr);
    }
    bus_info.unique_stops_count = unique_stops.size();
    bus_info.length = ComputeFullRouteDistance(bus);
    bus_info.stops_count = (bus.route_type == Bus::RouteType::Full) ? stops.size() : (stops.size() * 2 - 1);
    return bus_info;
}

// Distance

TransportCatalogue::BusInfo::Length TransportCatalogue::ComputeFullRouteDistance(const Bus& bus) const {
    using ranges::Zip;
    using ranges::Drop;
    using ranges::Reverse;

    BusInfo::Length length;
    kahan_algorithm::Summation<geo::Meter> sum_route_length;
    kahan_algorithm::Summation<geo::Meter> sum_geo_length;

    const auto& stops = bus.stops;
    for (const auto [stop_ptr_from, stop_ptr_to] : Zip(stops, Drop(stops, 1))) {
        sum_route_length += GetDistance(*stop_ptr_from, *stop_ptr_to).value_or(geo::Meter{0});
        sum_geo_length += GetGeoDistance(*stop_ptr_from, *stop_ptr_to);
    }
    length.geo = sum_geo_length.Get();

    if (bus.route_type == Bus::RouteType::Half) {
        length.geo *= 2;
        for (const auto [stop_ptr_from, stop_ptr_to] : Zip(Reverse(stops), Drop(Reverse(stops), 1))) {
            sum_route_length += GetDistance(*stop_ptr_from, *stop_ptr_to).value_or(geo::Meter{0});
        }
    }
    length.route = sum_route_length.Get();

    return length;
}

std::size_t TransportCatalogue::StopPtrPairHasher::operator()(std::pair<StopPtr, StopPtr> stops) const noexcept {
    static std::hash<StopPtr> stop_hash;
    constexpr unsigned int prime_num = 37;
    return stop_hash(stops.first) + prime_num * stop_hash(stops.second);
}

std::optional<geo::Meter> TransportCatalogue::GetDistance(const Stop& from, const Stop& to) const {
    geo::Meter distance;
    if (auto from_to_iter = distances_.find({&from, &to}); from_to_iter != distances_.end()) {
        distance = from_to_iter->second;
    } else if (auto to_from_iter = distances_.find({&to, &from}); to_from_iter != distances_.end()) {
        distance = to_from_iter->second;
        distances_.emplace(std::make_pair(&from, &to), distance);
    } else {
        return std::nullopt;
    }
    return distance;
}

} // namespace transport_catalogue