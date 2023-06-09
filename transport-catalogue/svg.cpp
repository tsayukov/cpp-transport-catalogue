#include "svg.h"

#include <algorithm>
#include <array>

namespace svg {

using namespace std::string_literals;
using namespace std::string_view_literals;

// Point

Point::Point(double x, double y) noexcept
        : x(x)
        , y(y) {
}

std::ostream& operator<<(std::ostream& output, Point point) {
    return output << point.x << ',' << point.y;
}

// Color

namespace color {

std::ostream& operator<<(std::ostream& output, Rgb rgb) {
    return output << static_cast<int>(rgb.red)   << ','
                  << static_cast<int>(rgb.green) << ','
                  << static_cast<int>(rgb.blue);
}

std::ostream& operator<<(std::ostream& output, Rgba rgba) {
    return output << static_cast<int>(rgba.red)   << ','
                  << static_cast<int>(rgba.green) << ','
                  << static_cast<int>(rgba.blue)  << ','
                  << rgba.opacity;
}

std::ostream& Print::operator()(std::monostate) const {
    return output << std::get<std::string>(color::None);
}

std::ostream& Print::operator()(const std::string& color_as_str) const {
    return output << (color_as_str.empty() ? std::get<std::string>(color::None) : color_as_str);
}

std::ostream& Print::operator()(Rgb rgb) const {
    return output << "rgb("sv << rgb << ")"sv;
}

std::ostream& Print::operator()(Rgba rgba) const {
    return output << "rgba(" << rgba << ")";
}

} // namespace color

std::ostream& operator<<(std::ostream& output, const color::Color& color) {
    std::visit(color::Print{ output }, color);
    return output;
}

// Path Properties

std::ostream& operator<<(std::ostream& output, StrokeLineCap stroke_linecap) {
    using namespace std::literals;

    switch (stroke_linecap) {
        case StrokeLineCap::BUTT: {
            return output << "butt"sv;
        }
        case StrokeLineCap::ROUND: {
            return output << "round"sv;
        }
        case StrokeLineCap::SQUARE: {
            return output << "square"sv;
        }
    }
    throw std::invalid_argument("No such enum"s);
}

std::ostream& operator<<(std::ostream& output, StrokeLineJoin stroke_linejoin) {
    using namespace std::literals;

    switch (stroke_linejoin) {
        case StrokeLineJoin::ARCS: {
            return output << "arcs"sv;
        }
        case StrokeLineJoin::BEVEL: {
            return output << "bevel"sv;
        }
        case StrokeLineJoin::MITER: {
            return output << "miter"sv;
        }
        case StrokeLineJoin::MITER_CLIP: {
            return output << "miter-clip"sv;
        }
        case StrokeLineJoin::ROUND: {
            return output << "round"sv;
        }
    }
    throw std::invalid_argument("No such enum"s);
}

// RenderContext

RenderContext::RenderContext(std::ostream& output) noexcept
        : output(output) {
}

RenderContext::RenderContext(std::ostream& output, int indent_step, int indent) noexcept
        : output(output)
        , indent_step(indent_step)
        , indent(indent) {
}

RenderContext RenderContext::Indented() const noexcept {
    return RenderContext(output, indent_step, indent + indent_step);
}

void RenderContext::RenderIndent() const {
    for (int i = 0; i < indent; ++i) {
        output.put(' ');
    }
}

// Object

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();
    RenderObject(context);
    context.output << '\n';
}

// Circle

Circle& Circle::SetCenter(Point center) noexcept {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius) noexcept {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& output = context.output;
    output << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\""sv;
    output << " r=\""sv << radius_ << "\""sv;
    RenderAttrs(output);
    output << " />"sv;
}

// Polyline

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& output = context.output;
    output << "<polyline points=\""sv;
    if (!points_.empty()) {
        output << points_.front();
        for (auto point_iter = ++points_.cbegin(); point_iter != points_.cend(); ++point_iter) {
            output << " "sv << *point_iter;
        }
    }
    output << "\""sv;
    RenderAttrs(output);
    output << " />"sv;
}

// Text

Text& Text::SetPosition(Point pos) noexcept {
    start_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) noexcept {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(std::uint32_t size) noexcept {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data) {
    data_ = std::move(data);
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& output = context.output;
    output << "<text x=\""sv << start_.x << "\" y=\""sv << start_.y << "\""sv;
    output << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv;
    output << " font-size=\""sv << font_size_ << "\""sv;
    if (!font_weight_.empty()) {
        output << " font-weight=\""sv << font_weight_ << "\""sv;
    }
    if (!font_family_.empty()) {
        output << " font-family=\""sv << font_family_ << "\""sv;
    }
    RenderAttrs(output);
    output << ">";
    EscapeText(output);
    output << "</text>"sv;
}

void Text::EscapeText(std::ostream& output) const {
    if (data_.empty()) {
        return;
    }

    struct EscapedChar {
        char original;
        std::string_view escaped;
    };

    constexpr std::array<EscapedChar, 5> escaped_chars = {
            EscapedChar{'\"', "&quot;"sv},
            EscapedChar{'\'', "&apos;"sv},
            EscapedChar{'<', "&lt;"sv},
            EscapedChar{'>', "&gt;"sv},
            EscapedChar{'&', "&amp;"sv},
    };

    using pos_t = std::string_view::size_type;
    using count_t = std::string_view::size_type;

    std::string_view text = data_;
    std::string_view replacement;
    while (!text.empty()) {
        auto iter = std::find_if(
                text.begin(), text.end(),
                [&escaped_chars, &replacement](char ch) noexcept {
                    for (const auto& escaped_char : escaped_chars) {
                        if (ch == escaped_char.original) {
                            replacement = escaped_char.escaped;
                            return true;
                        }
                    }
                    return false;
        });
        count_t n_before_escaped_char = iter - text.begin();
        output << text.substr(pos_t(0), n_before_escaped_char);
        if (iter != text.end()) {
            output << replacement;
            text.remove_prefix(1);
        }
        text.remove_prefix(n_before_escaped_char);
    }
}

// Document

void Document::AddPtr(std::unique_ptr<Object>&& object) {
    objects_.push_back(std::move(object));
}

void Document::Render(std::ostream& output) const {
    const RenderContext context(output, 4, 0);
    context.RenderIndent();
    output << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << '\n';
    output << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << '\n';
    for (const auto& object : objects_) {
        object->Render(context.Indented());
    }
    context.RenderIndent();
    output << "</svg>"sv;
}

}  // namespace svg