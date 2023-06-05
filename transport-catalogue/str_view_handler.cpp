#include "str_view_handler.h"

namespace transport_catalogue::str_view_handler {

[[nodiscard]] bool IsSpace(char ch) noexcept {
    return std::isspace(static_cast<unsigned char>(ch));
}

[[nodiscard]] bool IsNotSpace(char ch) noexcept {
    return !IsSpace(ch);
}

[[nodiscard]] std::string_view LeftStrip(std::string_view text) noexcept {
    auto first_non_whitespace = std::find_if(text.cbegin(), text.cend(), IsNotSpace);
    const auto count = static_cast<std::string_view::size_type>(first_non_whitespace - text.cbegin());
    text.remove_prefix(count);
    return text;
}

[[nodiscard]] std::string_view RightStrip(std::string_view text) noexcept {
    auto last_non_whitespace = std::find_if(text.crbegin(), text.crend(), IsNotSpace);
    const auto count = static_cast<std::string_view::size_type>(last_non_whitespace - text.crbegin());
    text.remove_suffix(count);
    return text;
}

[[nodiscard]] std::string_view Strip(std::string_view text) noexcept {
    return RightStrip(LeftStrip(text));
}

// SplitView methods definitions

SplitView::SplitView(std::string_view text) noexcept
        : text_(text) {
}

[[nodiscard]] bool SplitView::Empty() const noexcept {
    return text_.empty();
}

[[nodiscard]] bool SplitView::CanSplit(char separator) const noexcept {
    sep_pos_ = { separator, text_.find(separator) };
    return sep_pos_.pos != unknown_sep_pos_.pos;
}

[[nodiscard]] std::string_view SplitView::Rest() const noexcept {
    return text_;
}

[[nodiscard]] std::string_view SplitView::NextSubstrBefore(char separator) noexcept {
    using pos_t = size_type;
    using count_t = size_type;

    const pos_t sep_pos = (separator == sep_pos_.sep) ? sep_pos_.pos : text_.find(separator);
    // Exception safety: 0 <= text.size()
    const std::string_view left = text_.substr(pos_t(0), count_t(sep_pos));
    const count_t n_chars_to_remove = (sep_pos == unknown_sep_pos_.pos) ? text_.size() : sep_pos + 1;
    text_.remove_prefix(n_chars_to_remove);
    sep_pos_ = unknown_sep_pos_;
    return left;
}

[[nodiscard]] std::string_view SplitView::NextSubstrBefore(std::string_view separator) noexcept {
    using pos_t = size_type;
    using count_t = size_type;

    const pos_t sep_pos = text_.find(separator);
    // Exception safety: 0 <= text.size()
    const std::string_view left = text_.substr(pos_t(0), count_t(sep_pos));
    const count_t n_chars_to_remove = (sep_pos == unknown_sep_pos_.pos) ? text_.size() : sep_pos + separator.size();
    text_.remove_prefix(n_chars_to_remove);
    sep_pos_ = unknown_sep_pos_;
    return left;
}

[[nodiscard]] std::string_view SplitView::NextStrippedSubstrBefore(char separator) noexcept {
    return Strip(NextSubstrBefore(separator));
}

[[nodiscard]] std::string_view SplitView::NextStrippedSubstrBefore(std::string_view separator) noexcept {
    return Strip(NextSubstrBefore(separator));
}

void SplitView::SkipSubstrBefore(char separator) noexcept {
    (void) NextSubstrBefore(separator);
}

void SplitView::SkipSubstrBefore(std::string_view separator) noexcept {
    (void) NextSubstrBefore(separator);
}

} // namespace transport_catalogue::str_view_handler