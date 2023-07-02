#pragma once

namespace kahan_algorithm {

/// Kahan summation algorithm
/// source: https://en.wikipedia.org/wiki/Kahan_summation_algorithm
template<typename FloatingPoint>
class Summation {
public:
    [[nodiscard]] FloatingPoint Get() const noexcept {
        return accumulator_;
    }

    Summation& operator+=(FloatingPoint value) noexcept {
        const auto diff = value - compensation_;
        const auto new_acc = accumulator_ + diff;
        compensation_ = (new_acc - accumulator_) - diff;
        accumulator_ = new_acc;
        return *this;
    }

private:
    FloatingPoint accumulator_ = {};
    FloatingPoint compensation_ = {};
};

} // namespace kahan_algorithm