/// \file
/// Auxiliary functions for working with \e std::string_view

#pragma once

#include <array>
#include <cctype>
#include <charconv>
#include <optional>
#include <string_view>

namespace str_view_handler {

[[nodiscard]] bool IsSpace(char ch) noexcept;
[[nodiscard]] bool IsNotSpace(char ch) noexcept;

[[nodiscard]] std::string_view LeftStrip(std::string_view text) noexcept;
[[nodiscard]] std::string_view RightStrip(std::string_view text) noexcept;
[[nodiscard]] std::string_view Strip(std::string_view text) noexcept;

class SplitView {
public:
    using size_type = std::string_view::size_type;

    explicit SplitView(std::string_view text) noexcept;

    [[nodiscard]] bool Empty() const noexcept;

    [[nodiscard]] bool CanSplit(char separator) const noexcept;

    template<typename ...CharType>
    [[nodiscard]] std::pair<char, bool> CanSplit(CharType... separators) const noexcept;

    [[nodiscard]] std::string_view Rest() const noexcept;

    [[nodiscard]] std::string_view NextSubstrBefore(char separator) noexcept;
    [[nodiscard]] std::string_view NextSubstrBefore(std::string_view separator) noexcept;

    [[nodiscard]] std::string_view NextStrippedSubstrBefore(char separator) noexcept;
    [[nodiscard]] std::string_view NextStrippedSubstrBefore(std::string_view separator) noexcept;

    void SkipSubstrBefore(char separator) noexcept;
    void SkipSubstrBefore(std::string_view separator) noexcept;

private:
    struct NextSeparatorPosition {
        char sep;
        size_type pos;
    };

    static constexpr NextSeparatorPosition unknown_sep_pos_ = {'\0', std::string_view::npos };

    std::string_view text_;
    mutable NextSeparatorPosition sep_pos_ = unknown_sep_pos_;
};

template<typename ...CharType>
[[nodiscard]] std::pair<char, bool> SplitView::CanSplit(CharType... separators) const noexcept {
    static_assert((std::is_same_v<CharType, char> && ...));

    const std::array<char, sizeof...(separators)> expanded_seps = {separators... };
    for (const char sep : expanded_seps) {
        if (sep_pos_.sep == sep && sep_pos_.pos != unknown_sep_pos_.pos) {
            return { sep_pos_.sep, true };
        }
    }

    for (size_type pos = 0; pos < text_.size(); ++pos) {
        for (const char sep : expanded_seps) {
            if (text_[pos] == sep) {
                sep_pos_ = { text_[pos], pos };
                return { sep_pos_.sep, true };
            }
        }
    }
    return { '\0', false };
}

template<typename T>
std::optional<T> ParseAs(std::string_view text) noexcept {
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

    if (text.size() > 1 && text[0] == '+' && text[1] != '-') {
        text.remove_prefix(1);
    }
    T value;
    // No way in GCC (C++17)
    auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec == std::errc::invalid_argument || result.ec == std::errc::result_out_of_range) {
        return std::nullopt;
    }
    return value;
}

namespace tests {

inline void TestLeftStrip();
inline void TestRightStrip();
inline void TestStrip();
inline void TestSplitView();

} // namespace tests

} // namespace str_view_handler

