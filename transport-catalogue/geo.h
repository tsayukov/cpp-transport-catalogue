/// \file
/// Functions for working with geographical coordinates

#pragma once

namespace geo {

using Deg = double;
using Rad = double;
using Meter = double;

struct Coordinates {
    Deg lat;
    Deg lng;

    [[nodiscard]] bool operator==(const Coordinates& other) const noexcept;

    [[nodiscard]] bool operator!=(const Coordinates& other) const noexcept;
};

[[nodiscard]] constexpr Rad AsRad(Deg deg) noexcept;

[[nodiscard]] Meter ComputeDistance(Coordinates from, Coordinates to) noexcept;

} // namespace geo