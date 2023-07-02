/// \file
/// Entities of TransportCatalogue domain

#pragma once

#include "geo.h"

#include <string>
#include <vector>

namespace transport_catalogue {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

using StopPtr = const Stop*;

struct Bus {
    enum class RouteType {
        Full,
        Half,
    };

    std::string name;
    std::vector<StopPtr> stops;
    RouteType route_type;
};

using BusPtr = const Bus*;

[[nodiscard]] geo::Meter GetGeoDistance(const Stop& from, const Stop& to) noexcept;

} // namespace transport_catalogue