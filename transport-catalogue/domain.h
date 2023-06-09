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

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
};

using StopPtr = const Stop*;
using BusPtr = const Bus*;

[[nodiscard]] geo::Meter GetGeoDistance(const Stop& from, const Stop& to) noexcept;

} // namespace transport_catalogue