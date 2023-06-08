#define _USE_MATH_DEFINES

#include "geo.h"

#include <cassert>
#include <cmath>

namespace geo {

Coordinates::Coordinates(Degree lat, Degree lng) noexcept
        : lat(lat)
        , lng(lng) {
    assert(lat >= -90 && lat <= 90);
    assert(lng >= -180 && lng <= 180);
}

bool Coordinates::operator==(const Coordinates& other) const noexcept {
    return lat == other.lat && lng == other.lng;
}

bool Coordinates::operator!=(const Coordinates& other) const noexcept {
    return !(*this == other);
}

constexpr Radian AsRadian(Degree deg) noexcept {
    constexpr Radian conversion_factor = M_PI / 180.0;
    return deg * conversion_factor;
}

Meter ComputeDistance(Coordinates from, Coordinates to) noexcept {
    using namespace std;

    if (from == to) {
        return Meter(0);
    }
    const double d1 = sin(AsRadian(from.lat)) * sin(AsRadian(to.lat));
    const double d2 = cos(AsRadian(from.lat)) * cos(AsRadian(to.lat)) * cos(AsRadian(abs(from.lng - to.lng)));
    const Radian distance = acos(d1 + d2);
    constexpr Meter mean_earth_radius = 6371000.0;
    return distance * mean_earth_radius;
}

} // namespace geo