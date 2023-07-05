/// \file
/// Wrapper for integral and floating point types by using public inheritance
/// from number_wrapper::Base<WrappedType, Self, AllowedOps...>

#pragma once

#include <cmath>
#include <iostream>
#include <limits>
#include <type_traits>

namespace number_wrapper {

template<typename Self, typename Rhs = Self>
struct Equal {

    friend constexpr bool operator==(Self self, Rhs rhs) noexcept {
        return static_cast<typename Self::WrappedType>(self) == static_cast<typename Self::WrappedType>(rhs);
    }

    friend constexpr bool operator!=(Self self, Rhs rhs) noexcept {
        return !(self == rhs);
    }
};

template<typename Self, typename Rhs = Self>
struct Ordering {

    friend constexpr bool operator<(Self self, Rhs rhs) noexcept {
        return static_cast<typename Self::WrappedType>(self) < static_cast<typename Self::WrappedType>(rhs);
    }

    friend constexpr bool operator>(Self self, Rhs rhs) noexcept {
        return static_cast<typename Self::WrappedType>(rhs) < static_cast<typename Self::WrappedType>(self);
    }

    friend constexpr bool operator<=(Self self, Rhs rhs) noexcept {
        return !(rhs < self);
    }

    friend constexpr bool operator>=(Self self, Rhs rhs) noexcept {
        return !(self < rhs);
    }
};

template<typename Self, typename Out = Self>
struct UnaryPlus {

    friend constexpr Out operator+(Self self) noexcept {
        return static_cast<Out>(+static_cast<typename Self::WrappedType>(self));
    }
};

template<typename Self, typename Out = Self>
struct UnaryMinus {

    friend constexpr Out operator-(Self self) noexcept {
        return static_cast<Out>(-static_cast<typename Self::WrappedType>(self));
    }
};

template<typename Self, typename Out = Self>
struct Increment {

    friend constexpr Out& operator++(Self& self) noexcept {
        ++static_cast<typename Self::WrappedType&>(self);
        return static_cast<Out&>(self);
    }

    friend constexpr Out operator++(Self& self, int) noexcept {
        const auto self_copy = self;
        ++self;
        return static_cast<Out>(self_copy);
    }
};

template<typename Self, typename Out = Self>
struct Decrement {

    friend constexpr Out& operator--(Self& self) noexcept {
        --static_cast<typename Self::WrappedType&>(self);
        return static_cast<Out&>(self);
    }

    friend constexpr Out operator--(Self& self, int) noexcept {
        const auto self_copy = self;
        --self;
        return static_cast<Out>(self_copy);
    }
};

template<typename Self, typename Rhs = Self, typename Out = Self>
struct Add {

    friend constexpr Out& operator+=(Self& self, Rhs rhs) noexcept {
        static_cast<typename Self::WrappedType&>(self) += static_cast<typename Self::WrappedType>(rhs);
        return static_cast<Out&>(self);
    }

    friend constexpr Out operator+(Self self, Rhs rhs) noexcept {
        auto self_copy = self;
        return self_copy += rhs;
    }
};

template<typename Self, typename Rhs = Self, typename Out = Self>
struct Subtract {

    friend constexpr Out& operator-=(Self& self, Rhs rhs) noexcept {
        static_cast<typename Self::WrappedType&>(self) -= static_cast<typename Self::WrappedType>(rhs);
        return static_cast<Out&>(self);
    }

    friend constexpr Out operator-(Self self, Rhs rhs) noexcept {
        auto self_copy = self;
        return self_copy -= rhs;
    }
};

template<typename Self, typename Rhs = Self, typename Out = Self>
struct Multiply {

    friend constexpr Out& operator*=(Self& self, Rhs rhs) noexcept {
        static_cast<typename Self::WrappedType&>(self) *= static_cast<typename Self::WrappedType>(rhs);
        return static_cast<Out&>(self);
    }

    friend constexpr Out operator*(Self self, Rhs rhs) noexcept {
        auto self_copy = self;
        return self_copy *= rhs;
    }
};

template<typename Self, typename Rhs = Self, typename Out = Self>
struct Divide {

    friend constexpr Out& operator/=(Self& self, Rhs rhs) noexcept {
        static_cast<typename Self::WrappedType&>(self) /= static_cast<typename Self::WrappedType>(rhs);
        return static_cast<Out&>(self);
    }

    friend constexpr Out operator/(Self self, Rhs rhs) noexcept {
        auto self_copy = self;
        return self_copy /= rhs;
    }
};

template<typename Self, typename Rhs = Self, typename Out = Self>
struct Remainder {

    friend constexpr Out& operator%=(Self& self, Rhs rhs) noexcept {
        if constexpr (std::is_floating_point_v<typename Self::WrappedType>) {
            self = Self{std::remainder(
                            static_cast<typename Self::WrappedType>(self),
                            static_cast<typename Self::WrappedType>(rhs))};
        } else {
            static_cast<typename Self::WrappedType&>(self) %= static_cast<typename Self::WrappedType>(rhs);
        }
        return static_cast<Out&>(self);
    }

    friend constexpr Out operator%(Self self, Rhs rhs) noexcept {
        auto self_copy = self;
        return self_copy %= rhs;
    }
};

template<typename Self, typename Out = Self>
struct BitwiseNot {

    friend constexpr Out operator~(Self self) noexcept {
        return static_cast<Out>(~static_cast<typename Self::WrappedType>(self));
    }
};

template<typename Self, typename Rhs = Self, typename Out = Self>
struct BitwiseAnd {

    friend constexpr Out& operator&=(Self& self, Rhs rhs) noexcept {
        static_cast<typename Self::WrappedType&>(self) &= static_cast<typename Self::WrappedType>(rhs);
        return static_cast<Out&>(self);
    }

    friend constexpr Out operator&(Self self, Rhs rhs) noexcept {
        auto self_copy = self;
        return self_copy &= rhs;
    }
};

template<typename Self, typename Rhs = Self, typename Out = Self>
struct BitwiseOr {

    friend constexpr Out& operator|=(Self& self, Rhs rhs) noexcept {
        static_cast<typename Self::WrappedType&>(self) |= static_cast<typename Self::WrappedType>(rhs);
        return static_cast<Out&>(self);
    }

    friend constexpr Out operator|(Self self, Rhs rhs) noexcept {
        auto self_copy = self;
        return self_copy |= rhs;
    }
};

template<typename Self, typename Rhs = Self, typename Out = Self>
struct BitwiseXor {

    friend constexpr Out& operator^=(Self& self, Rhs rhs) noexcept {
        static_cast<typename Self::WrappedType&>(self) ^= static_cast<typename Self::WrappedType>(rhs);
        return static_cast<Out&>(self);
    }

    friend constexpr Out operator^(Self self, Rhs rhs) noexcept {
        auto self_copy = self;
        return self_copy ^= rhs;
    }
};

template<typename Self, typename Rhs = Self, typename Out = Self>
struct BitwiseLeftShift {

    friend constexpr Out& operator<<=(Self& self, Rhs rhs) noexcept {
        static_cast<typename Self::WrappedType&>(self) <<= static_cast<typename Self::WrappedType>(rhs);
        return static_cast<Out&>(self);
    }

    friend constexpr Out operator<<(Self self, Rhs rhs) noexcept {
        auto self_copy = self;
        return self_copy <<= rhs;
    }
};

template<typename Self, typename Rhs = Self, typename Out = Self>
struct BitwiseRightShift {

    friend constexpr Out& operator>>=(Self& self, Rhs rhs) noexcept {
        static_cast<typename Self::WrappedType&>(self) >>= static_cast<typename Self::WrappedType>(rhs);
        return static_cast<Out&>(self);
    }

    friend constexpr Out operator>>(Self self, Rhs rhs) noexcept {
        auto self_copy = self;
        return self_copy >>= rhs;
    }
};

template<typename T, typename Self, typename... AllowedOps>
class Base : private AllowedOps... {
public:
    using WrappedType = T;

    static_assert(std::is_integral_v<WrappedType> || std::is_floating_point_v<WrappedType>);

    constexpr explicit Base(WrappedType value = {}) noexcept
            : value_(value) {
        static_assert(std::is_base_of_v<Base, Self>);
        static_assert(sizeof(Base) == sizeof(Self));
    }

    constexpr explicit operator WrappedType() const noexcept {
        return value_;
    }

    constexpr explicit operator const WrappedType&() const noexcept {
        return value_;
    }

    constexpr explicit operator WrappedType&() noexcept {
        return value_;
    }

    constexpr WrappedType Get() const noexcept {
        return value_;
    }

    constexpr WrappedType& Get() noexcept {
        return value_;
    }

    friend std::istream& operator>>(std::istream& input, Base& base) {
        return input << static_cast<WrappedType&>(base);
    }

    friend std::ostream& operator<<(std::ostream& output, Base base) {
        return output << static_cast<WrappedType>(base);
    }

private:
    WrappedType value_;
};

namespace tests {

void TestEqual();
void TestOrdering();
void TestUnaryPlus();
void TestUnaryMinus();
void TestIncrement();
void TestDecrement();
void TestAdd();
void TestSubtract();
void TestMultiply();
void TestDivide();
void TestRemainder();
void TestBitwiseNot();
void TestBitwiseAnd();
void TestBitwiseOr();
void TestBitwiseXor();
void TestBitwiseLeftShift();
void TestBitwiseRightShift();

}

} // namespace number_wrapper