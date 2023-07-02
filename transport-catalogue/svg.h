/// \file
/// Print limited number of svg objects (Circle, Polyline, and Text)

#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg {

// Point

struct Point {
    double x = 0;
    double y = 0;

    Point() noexcept = default;
    Point(double x, double y) noexcept;
};

std::ostream& operator<<(std::ostream& output, Point point);

// Color

namespace color {

struct Rgb {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

std::ostream& operator<<(std::ostream& output, Rgb rgb);

struct Rgba {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0;
};

std::ostream& operator<<(std::ostream& output, color::Rgba rgba);

struct Print {
    std::ostream& output;

    std::ostream& operator()(std::monostate) const;
    std::ostream& operator()(const std::string& color_as_str) const;
    std::ostream& operator()(svg::color::Rgb rgb) const;
    std::ostream& operator()(svg::color::Rgba rgba) const;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

inline const Color None("none");

} // namespace color

std::ostream& operator<<(std::ostream& output, const color::Color& color);

// Path Properties

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

std::ostream& operator<<(std::ostream& output, StrokeLineCap stroke_linecap);

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream& operator<<(std::ostream& output, StrokeLineJoin stroke_linejoin);

template<typename Owner>
class PathProps {
public:
    using Color = color::Color;

    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeWidth(double width) noexcept {
        stroke_width_ = width;
        return AsOwner();
    }

    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_linecap_ = line_cap;
        return AsOwner();
    }

    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_linejoin_ = line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using namespace std::string_view_literals;

        if (fill_color_.has_value()) {
            out << " fill=\""sv << fill_color_.value() << "\""sv;
        }
        if (stroke_color_.has_value()) {
            out << " stroke=\""sv << stroke_color_.value() << "\""sv;
        }
        if (stroke_width_.has_value()) {
            out << " stroke-width=\""sv << stroke_width_.value() << "\""sv;
        }
        if (stroke_linecap_.has_value()) {
            out << " stroke-linecap=\""sv << stroke_linecap_.value() << "\""sv;
        }
        if (stroke_linejoin_.has_value()) {
            out << " stroke-linejoin=\""sv << stroke_linejoin_.value() << "\""sv;
        }
    }

private:
    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_linecap_;
    std::optional<StrokeLineJoin> stroke_linejoin_;

    Owner& AsOwner() noexcept {
        return static_cast<Owner&>(*this);
    }
};

// RenderContext

struct RenderContext {
    std::ostream& output;
    int indent_step;
    int indent;

    explicit RenderContext(std::ostream& output) noexcept;
    RenderContext(std::ostream& output, int indent_step, int indent = 0) noexcept;

    [[nodiscard]] RenderContext Indented() const noexcept;

    void RenderIndent() const;
};

// Object

class Object {
public:
    virtual ~Object() = default;
    void Render(const RenderContext& context) const;
private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

/// Object Circle
/// source: https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center) noexcept;
    Circle& SetRadius(double radius) noexcept;
private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

/// Object Polyline
/// source: https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
class Polyline final : public Object, public PathProps<Polyline> {
public:
    Polyline& AddPoint(Point point);
private:
    void RenderObject(const RenderContext& context) const override;

    std::vector<Point> points_;
};

/// Object Text
/// source: https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
class Text final : public Object, public PathProps<Text> {
public:
    Text& SetPosition(Point pos) noexcept;
    Text& SetOffset(Point offset) noexcept;
    Text& SetFontSize(std::uint32_t size) noexcept;
    Text& SetFontFamily(std::string font_family);
    Text& SetFontWeight(std::string font_weight);
    Text& SetData(std::string data);
private:
    Point start_ = {0.0, 0.0};
    Point offset_ = {0.0, 0.0};
    std::uint32_t font_size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;

    void RenderObject(const RenderContext& context) const override;
    void EscapeText(std::ostream& output) const;
};

class ObjectContainer {
public:
    virtual ~ObjectContainer() = default;

    template<typename Obj>
    void Add(Obj object) {
        static_assert(std::is_base_of_v<Object, Obj>);
        AddPtr(std::make_unique<Obj>(std::move(object)));
    }

    virtual void AddPtr(std::unique_ptr<Object>&& object) = 0;
};

class Drawable {
public:
    virtual ~Drawable() = default;
    virtual void Draw(ObjectContainer& container) const = 0;
};

class Document final : public ObjectContainer {
public:
    void AddPtr(std::unique_ptr<Object>&& object) override;
    void Render(std::ostream& output) const;
private:
    std::vector<std::unique_ptr<Object>> objects_;
};

} // namespace svg