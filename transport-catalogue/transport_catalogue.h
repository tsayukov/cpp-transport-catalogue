/// \file
/// Database for storing information about bus routes and stops

#pragma once

#include "geo.h"

#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace transport_catalogue {

struct Bus;

struct Stop {
    std::string name;
    Coordinates coordinates;

    struct Statistics {
        std::vector<const Bus*> buses;
        bool is_buses_unique_and_ordered = false;
    };
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;

    struct Statistics {
        unsigned int stops_count;
        unsigned int unique_stops_count;
        Meter route_length;
        Meter geo_length;
    };
};

class TransportCatalogue {
public:
    void AddStop(Stop stop);
    void AddBus(Bus bus);

    void SetDistance(std::string_view stop_name_from, std::string_view stop_name_to, Meter distance);

    std::optional<Meter> GetDistance(std::string_view stop_name_from, std::string_view stop_name_to) const;

    [[nodiscard]] std::optional<const Stop*> FindStopBy(std::string_view stop_name) const;
    [[nodiscard]] std::optional<const Bus*> FindBusBy(std::string_view bus_name) const;

    // Statistics

    struct StopInfo {
        std::string_view stop_name;
        std::optional<std::vector<const Bus*>> buses;
    };

    [[nodiscard]] StopInfo GetStopInfo(std::string_view stop_name) const;

    struct BusInfo {
        std::string_view bus_name;
        std::optional<Bus::Statistics> statistics;
    };

    [[nodiscard]] BusInfo GetBusInfo(std::string_view bus_name) const;

private:
    template<typename Value>
    using Indices = std::unordered_map<std::string_view, const Value*>;

    std::deque<Stop> stops_;
    Indices<Stop> stop_indices_;

    std::deque<Bus> buses_;
    Indices<Bus> bus_indices_;

    // Statistics

    mutable std::unordered_map<const Stop*, Stop::Statistics> stop_statistics_;
    mutable std::unordered_map<const Bus*, Bus::Statistics> bus_statistics_;

    const Bus::Statistics& ComputeBusStatistics(const Bus* bus_ptr) const;

    // Distance

    struct StopPtrsHasher {
        std::size_t operator()(std::pair<const Stop*, const Stop*> stops) const noexcept;
    };

    mutable std::unordered_map<std::pair<const Stop*, const Stop*>, Meter, StopPtrsHasher> distances_;

    [[nodiscard]] Meter GetDistanceBetween(const Stop* from, const Stop* to) const;
    [[nodiscard]] Meter GetGeoDistanceBetween(const Stop* from, const Stop* to) const noexcept;
};

} // namespace transport_catalogue