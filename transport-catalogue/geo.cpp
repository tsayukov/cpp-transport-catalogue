#include "geo.h"

#include <cassert>
#include <cmath>

namespace geo {

// Degree

constexpr Degree::WrappedType Degree::AsRadian() const noexcept {
    constexpr double pi = 3.14159265358979323846;
    constexpr double conversion_factor = pi / 180.0;
    return (*this * conversion_factor).Get();
}

// Coordinates

Coordinates::Coordinates(Degree lat, Degree lng) noexcept
        : lat(lat)
        , lng(lng) {
    // todo when do the check?
    assert(lat >= Degree{-90.0} && lat <= Degree{90.0});
    assert(lng >= Degree{-180.0} && lng <= Degree{180.0});
}

bool Coordinates::operator==(const Coordinates& other) const noexcept {
    return lat == other.lat && lng == other.lng;
}

bool Coordinates::operator!=(const Coordinates& other) const noexcept {
    return !(*this == other);
}

Meter ComputeDistance(Coordinates from, Coordinates to) noexcept {
    using namespace std;

    if (from == to) {
        return Meter{0};
    }

    constexpr auto mean_earth_radius = Meter{6'371'000.0};

    const auto d1 = sin(from.lat.AsRadian()) * sin(to.lat.AsRadian());
    const auto diff = Degree{std::abs((from.lng - to.lng).Get())};
    const auto d2 = cos(from.lat.AsRadian()) * cos(to.lat.AsRadian()) * cos(diff.AsRadian());
    const auto distance_factor = acos(d1 + d2);

    return mean_earth_radius * distance_factor;
}

} // namespace geo