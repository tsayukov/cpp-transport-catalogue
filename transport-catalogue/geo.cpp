#define _USE_MATH_DEFINES

#include "geo.h"

#include <cmath>

namespace geo {

bool Coordinates::operator==(const Coordinates& other) const noexcept {
    return lat == other.lat && lng == other.lng;
}

bool Coordinates::operator!=(const Coordinates& other) const noexcept {
    return !(*this == other);
}

constexpr Rad AsRad(Deg deg) noexcept {
    constexpr Rad conversion_factor = M_PI / 180.0;
    return deg * conversion_factor;
}

Meter ComputeDistance(Coordinates from, Coordinates to) noexcept {
    using namespace std;

    if (from == to) {
        return Meter(0);
    }
    constexpr Meter mean_earth_radius = 6371000.0;
    return mean_earth_radius * acos(sin(AsRad(from.lat)) * sin(AsRad(to.lat))
                                  + cos(AsRad(from.lat)) * cos(AsRad(to.lat)) * cos(AsRad(abs(from.lng - to.lng))));
}

} // namespace geo