/// \file
/// Functions for working with geographical coordinates

#pragma once

#include <cmath>

namespace transport_catalogue::geo {

using Deg = double;
using Rad = double;
using Meter = double;

struct Coordinates {
    Deg lat;
    Deg lng;

    bool operator==(const Coordinates& other) const noexcept {
        return lat == other.lat && lng == other.lng;
    }

    bool operator!=(const Coordinates& other) const noexcept {
        return !(*this == other);
    }
};

inline constexpr Rad AsRad(Deg deg) noexcept {
    // C++17 still doesn't have PI constant. GCC has M_PI macros, but not MSVC.
    // Can't wait to use C++20 <numbers>!
    constexpr Rad pi = 3.1415926535;
    constexpr Rad conversion_factor = pi / 180.0;
    return deg * conversion_factor;
}

inline Meter ComputeDistance(Coordinates from, Coordinates to) noexcept {
    using namespace std;

    if (from == to) {
        return Meter(0);
    }
    constexpr Meter mean_earth_radius = 6371000.0;
    return mean_earth_radius * acos(sin(AsRad(from.lat)) * sin(AsRad(to.lat))
                                  + cos(AsRad(from.lat)) * cos(AsRad(to.lat)) * cos(AsRad(abs(from.lng - to.lng))));
}

} // namespace transport_catalogue::geo