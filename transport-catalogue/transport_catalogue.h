/// \file
/// Database for storing information about bus routes and stops

#pragma once

#include "geo.h"
#include "domain.h"
#include "ranges.h"

#include <algorithm>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace transport_catalogue {

class TransportCatalogue final {
private:
    using StopIterator = typename std::deque<Stop>::const_iterator;
    using BusIterator = typename std::deque<Bus>::const_iterator;

public:
    void AddStop(Stop stop);
    void AddBus(Bus bus);

    template<typename StopContainer>
    void AddBus(std::string name, const StopContainer& stop_names, Bus::RouteType route_type);

    void SetDistanceBetweenStops(std::string_view from, std::string_view to, geo::Meter distance);
    [[nodiscard]] std::optional<geo::Meter> GetDistanceBetweenStops(std::string_view from, std::string_view to) const;

    [[nodiscard]] std::optional<StopPtr> FindStopBy(std::string_view stop_name) const;
    [[nodiscard]] std::optional<BusPtr> FindBusBy(std::string_view bus_name) const;

    using StopRange = ranges::ConstRange<StopIterator>;
    using BusRange = ranges::ConstRange<BusIterator>;

    [[nodiscard]] StopRange GetAllStops() const noexcept;
    [[nodiscard]] BusRange GetAllBuses() const noexcept;

    // Statistics

    struct StopInfo {
        std::vector<BusPtr> buses;
    };

    [[nodiscard]] std::optional<const StopInfo*> GetStopInfo(std::string_view stop_name) const;

    struct BusInfo {
        struct Length {
            geo::Meter route;
            geo::Meter geo;
        };

        unsigned int stops_count = 0;
        unsigned int unique_stops_count = 0;
        Length length;
    };

    [[nodiscard]] std::optional<const BusInfo*> GetBusInfo(std::string_view bus_name) const;

private:
    template<typename Value>
    using Indices = std::unordered_map<std::string_view, Value>;

    std::deque<Stop> stops_;
    Indices<StopPtr> stop_indices_;

    std::deque<Bus> buses_;
    Indices<BusPtr> bus_indices_;

    // Statistics

    struct StopInfoStorage {
        StopInfo stop_info;
        bool is_buses_sorted_and_unique = false;
    };

    mutable std::unordered_map<StopPtr, StopInfoStorage> stop_infos_;
    mutable std::unordered_map<BusPtr, BusInfo> bus_infos_;

    static const StopInfo& PrepareStopInfo(StopInfoStorage& stop_info_storage);

    const BusInfo& ComputeBusInfo(const Bus& bus) const;

    // Distance

    BusInfo::Length ComputeFullRouteDistance(const Bus& bus) const;

    struct StopPtrPairHasher {
        std::size_t operator()(std::pair<StopPtr, StopPtr> stops) const noexcept;
    };

    mutable std::unordered_map<std::pair<StopPtr, StopPtr>, geo::Meter, StopPtrPairHasher> distances_;

    [[nodiscard]] std::optional<geo::Meter> GetDistance(const Stop& from, const Stop& to) const;
};

template<typename StopContainer>
void TransportCatalogue::AddBus(std::string name, const StopContainer& stop_names, Bus::RouteType route_type) {
    Bus bus;
    bus.name = std::move(name);
    bus.route_type = route_type;
    bus.stops.reserve(stop_names.size());
    for (const auto& stop_name : stop_names) {
        if (auto stop = FindStopBy(stop_name)) {
            bus.stops.push_back(stop.value());
        } else {
            using namespace std::string_literals;
            throw std::domain_error("Error occurs during creation of a bus: there is no such stop in the database"s);
        }
    }
    AddBus(std::move(bus));
}

} // namespace transport_catalogue