#pragma once

#include <functional>
#include <iterator>
#include <tuple>
#include <type_traits>

namespace ranges {

template<typename Iter>
constexpr bool IsConstIterator() noexcept {
    using ValType = typename std::iterator_traits<Iter>::value_type;
    using DerefType = decltype(*std::declval<Iter>());

    if constexpr (std::is_same_v<DerefType, ValType>) {
        return true;
    }
    if constexpr (std::is_same_v<std::decay_t<DerefType>, ValType>) {
        return !std::is_same_v<std::remove_reference_t<DerefType>, ValType>;
    }
    return !std::is_convertible_v<DerefType, std::add_lvalue_reference<ValType>>;
}

template<typename Iter>
class Range;

template<typename Iter>
class ConstRange : public Range<Iter> {
public:
    static_assert(IsConstIterator<Iter>());

    constexpr ConstRange(Iter first, Iter last) noexcept(std::is_nothrow_copy_constructible_v<Iter>)
            : Range<Iter>(first, last) {
    }

    constexpr ConstRange(Range<Iter> other) noexcept(std::is_nothrow_copy_constructible_v<Iter>)
            : Range<Iter>(other) {
    }

    template<typename Container>
    constexpr /* implicit */ ConstRange(const Container& container)
            noexcept(noexcept(Range<Iter>(container.begin(), container.end())))
            : Range<Iter>(container.begin(), container.end()) {
    }
};

template<typename Iter>
class Range {
public:
    using value_type = typename std::iterator_traits<Iter>::value_type;

    using iterator = Iter;
    using const_iterator = std::conditional_t<IsConstIterator<Iter>(), Iter, void>;

    const Iter first;
    const Iter last;

    constexpr Range(Iter first, Iter last) noexcept(std::is_nothrow_copy_constructible_v<Iter>)
            : first(first)
            , last(last) {
    }

    Range(ConstRange<Iter>) = delete;

    template<typename Container>
    constexpr /* implicit */ Range(Container& container)
            noexcept(noexcept(Range<Iter>(container.begin(), container.end())))
            : Range<Iter>(container.begin(), container.end()) {
    }

    constexpr Iter begin() const noexcept(std::is_nothrow_copy_constructible_v<Iter>) {
        return first;
    }

    constexpr Iter end() const noexcept(std::is_nothrow_copy_constructible_v<Iter>) {
        return last;
    }
};

template<typename Container>
constexpr auto AsConstRange(const Container& container)
        noexcept(noexcept(ConstRange(container.begin(), container.end()))) {
    return ConstRange(container.begin(), container.end());
}

template<typename Container>
auto Reverse(const Container& container) {
    return ConstRange(container.rbegin(), container.rend());
}

template<typename Integral>
class CountIterator {
public:
    static_assert(std::is_integral_v<Integral>);

    using iterator_category = void;
    using value_type = Integral;
    using difference_type = void;
    using pointer = void;
    using reference = value_type;

    constexpr explicit CountIterator(Integral from = {}) noexcept
            : current_(from) {
    }

    constexpr bool operator==(CountIterator rhs) const noexcept {
        return current_ == rhs.current_;
    }

    constexpr bool operator!=(CountIterator rhs) const noexcept {
        return !(*this == rhs);
    }

    constexpr reference operator*() const noexcept {
        return current_;
    }

    constexpr CountIterator& operator++() noexcept {
        ++current_;
        return *this;
    }

    constexpr CountIterator operator++(int) noexcept {
        const auto this_copy = *this;
        ++current_;
        return this_copy;
    }

private:
    Integral current_;
};

constexpr auto Indices(std::size_t from, std::size_t to) noexcept {
    using SizeCountIterator = CountIterator<std::size_t>;

    if (from >= to) {
        to = from;
    }
    return ConstRange(SizeCountIterator(from), SizeCountIterator(to));
}

constexpr auto Indices(std::size_t to) noexcept {
    return Indices(0, to);
}

template<typename FirstIter, typename SecondIter>
class ZipIterator {
private:
    using FirstDerefReturnType = decltype(*std::declval<FirstIter>());
    using SecondDerefReturnType = decltype(*std::declval<SecondIter>());
    using FirstValueType = typename std::iterator_traits<FirstIter>::value_type;
    using SecondValueType = typename std::iterator_traits<SecondIter>::value_type;
    using FirstIterCategory = typename std::iterator_traits<FirstIter>::iterator_category;
    using SecondIterCategory = typename std::iterator_traits<SecondIter>::iterator_category;
public:
    static_assert(!std::is_same_v<FirstIterCategory, std::input_iterator_tag>);
    static_assert(!std::is_same_v<SecondIterCategory, std::input_iterator_tag>);

    using iterator_category = void;
    using value_type = std::tuple<FirstValueType, SecondValueType>;
    using difference_type = void;
    using pointer = void;
    using reference = std::tuple<FirstDerefReturnType, SecondDerefReturnType>;

    ZipIterator()
    noexcept(std::is_nothrow_default_constructible_v<FirstIter>
             && std::is_nothrow_default_constructible_v<SecondIter>) = default;

    ZipIterator(FirstIter first, SecondIter second)
    noexcept(std::is_nothrow_copy_constructible_v<FirstIter> && std::is_nothrow_copy_constructible_v<SecondIter>)
            : first_(first)
            , second_(second) {
    }

    bool operator==(const ZipIterator& rhs) const
        noexcept(noexcept(std::declval<FirstIter>() == rhs.first_ && std::declval<SecondIter>() == rhs.second_)) {
        return first_ == rhs.first_ && second_ == rhs.second_;
    }

    bool operator!=(const ZipIterator& rhs) const noexcept(noexcept(*this == rhs)) {
        return !(*this == rhs);
    }

    reference operator*() {
        constexpr bool is_first_ref = std::is_reference_v<FirstDerefReturnType>;
        constexpr bool is_second_ref = std::is_reference_v<SecondDerefReturnType>;

        if constexpr (is_first_ref && is_second_ref) {
            return std::tie(*first_, *second_);
        }
        if constexpr (is_first_ref && !is_second_ref) {
            return std::make_tuple(std::ref(*first_), *second_);
        }
        if constexpr (!is_first_ref && is_second_ref) {
            return std::make_tuple(*first_, std::ref(*second_));
        } else {
            return std::make_tuple(*first_, *second_);
        }
    }

    ZipIterator& operator++()
        noexcept(noexcept(++std::declval<FirstIter>()) && noexcept(++std::declval<SecondIter>() )) {
        ++first_;
        ++second_;
        return *this;
    }

private:
    FirstIter first_;
    SecondIter second_;
};

template<typename FirstContainer, typename SecondContainer>
auto Zip(const FirstContainer& first_container, const SecondContainer& second_container) {
    const auto first_distance = std::distance(first_container.begin(), first_container.end());
    const auto second_distance = std::distance(second_container.begin(), second_container.end());
    const auto distance = std::min(first_distance, second_distance);

    auto first_range_begin = first_container.begin();
    auto second_range_begin = second_container.begin();

    auto first_range_end = std::next(first_range_begin, distance);
    auto second_range_end = std::next(second_range_begin, distance);

    return ConstRange{ZipIterator{first_range_begin, second_range_begin},
                      ZipIterator{first_range_end, second_range_end}};
}

template<typename Container>
auto Drop(const Container& container, unsigned int drop_count) {
    using IterCategory = typename std::iterator_traits<typename Container::const_iterator>::iterator_category;

    auto range_begin = container.begin();
    auto range_end = container.end();
    if constexpr (!std::is_same_v<IterCategory, std::random_access_iterator_tag>) {
        for ([[maybe_unused]] auto _ : Indices(drop_count)) {
            if (range_begin == range_end) {
                break;
            }
            ++range_begin;
        }
    } else if (const auto distance = range_end - range_begin;
               drop_count < static_cast<unsigned int>(distance)) {
        range_begin += drop_count;
    } else {
        range_begin = range_end;
    }
    return ConstRange{range_begin, range_end};
}

} // namespace ranges