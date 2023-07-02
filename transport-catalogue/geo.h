/// \file
/// Functions for working with geographical coordinates

#pragma once

#include "number_wrapper.h"

namespace geo {

using namespace number_wrapper;

class Degree final
        : public number_wrapper::Base<double, Degree,
                Equal<Degree>, Ordering<Degree>,
                UnaryPlus<Degree>, UnaryMinus<Degree>,
                Add<Degree>, Subtract<Degree>,
                Multiply<Degree, double>, Divide<Degree, double>, Divide<Degree, Degree, double>> {
public:
    using Base::Base;

    [[nodiscard]] constexpr WrappedType AsRadian() const noexcept;
};

class Meter final
        : public number_wrapper::Base<double, Meter,
                Equal<Meter>, Ordering<Meter>,
                UnaryPlus<Meter>,
                Add<Meter>, Subtract<Meter>,
                Multiply<Meter, double>, Divide<Meter, double>, Divide<Meter, Meter, double>> {
public:
    using Base::Base;
};

struct Coordinates {
    Degree lat;
    Degree lng;

    Coordinates() noexcept = default;
    Coordinates(Degree lat, Degree lng) noexcept;

    [[nodiscard]] bool operator==(const Coordinates& other) const noexcept;
    [[nodiscard]] bool operator!=(const Coordinates& other) const noexcept;
};

[[nodiscard]] Meter ComputeDistance(Coordinates from, Coordinates to) noexcept;

} // namespace geo