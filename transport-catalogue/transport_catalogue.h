/// \file
/// Database for storing information about bus routes and stops

#pragma once

#include "geo.h"
#include "domain.h"

#include <deque>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace transport_catalogue {

using StopPtr = const Stop*;
using BusPtr = const Bus*;

class TransportCatalogue final {
public:
    void AddStop(const Stop& stop);
    void AddStop(Stop&& stop);

    void AddBus(const Bus& bus);
    void AddBus(Bus&& bus);

    void SetDistance(std::string_view stop_name_from, std::string_view stop_name_to, geo::Meter distance);

    [[nodiscard]] std::optional<geo::Meter> GetDistance(std::string_view stop_name_from,
                                                        std::string_view stop_name_to) const;

    [[nodiscard]] std::optional<StopPtr> FindStopBy(std::string_view stop_name) const;
    [[nodiscard]] std::optional<BusPtr> FindBusBy(std::string_view bus_name) const;

    // Statistics

    struct StopInfo {
    public:
        using Buses = std::vector<BusPtr>;

        std::string_view stop_name;
        std::optional<Buses> buses;

        StopInfo(std::string_view stop_name, std::optional<Buses> buses);

    private:
        friend TransportCatalogue;
        bool is_buses_unique_and_ordered = false;
    };

    [[nodiscard]] StopInfo GetStopInfo(std::string_view stop_name) const;

    struct BusInfo {
        struct Statistics {
            unsigned int stops_count;
            unsigned int unique_stops_count;
            geo::Meter route_length;
            geo::Meter geo_length;
        };

        std::string_view bus_name;
        std::optional<Statistics> statistics;
    };

    [[nodiscard]] BusInfo GetBusInfo(std::string_view bus_name) const;

private:
    template<typename Value>
    using Indices = std::unordered_map<std::string_view, Value>;

    std::deque<Stop> stops_;
    Indices<StopPtr> stop_indices_;

    std::deque<Bus> buses_;
    Indices<BusPtr> bus_indices_;

    // Statistics

    mutable std::unordered_map<StopPtr, StopInfo> stop_statistics_;
    mutable std::unordered_map<BusPtr, BusInfo> bus_statistics_;

    const BusInfo& ComputeBusStatistics(const Bus& bus) const;

    // Distance

    struct StopPtrPairHasher {
        std::size_t operator()(std::pair<StopPtr, StopPtr> stops) const noexcept;
    };

    mutable std::unordered_map<std::pair<StopPtr, StopPtr>, geo::Meter, StopPtrPairHasher> distances_;

    [[nodiscard]] geo::Meter GetDistance(const Stop& from, const Stop& to) const;
};

} // namespace transport_catalogue