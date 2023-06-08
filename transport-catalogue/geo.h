/// \file
/// Functions for working with geographical coordinates

#pragma once

namespace geo {

using Degree = double;
using Radian = double;
using Meter = double;

struct Coordinates {
    Degree lat = 0.0;
    Degree lng = 0.0;

    Coordinates() noexcept = default;
    Coordinates(Degree lat, Degree lng) noexcept;

    [[nodiscard]] bool operator==(const Coordinates& other) const noexcept;

    [[nodiscard]] bool operator!=(const Coordinates& other) const noexcept;
};

[[nodiscard]] constexpr Radian AsRadian(Degree deg) noexcept;

[[nodiscard]] Meter ComputeDistance(Coordinates from, Coordinates to) noexcept;

} // namespace geo