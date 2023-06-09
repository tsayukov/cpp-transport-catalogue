/// \file
/// \todo add a description

#pragma once

#include "geo.h"
#include "svg.h"
#include "domain.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <unordered_set>

namespace transport_catalogue::renderer {

inline constexpr double EPSILON = 1e-6;

bool IsZero(double value);

class SphereProjector final {
public:
    template<typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
            : padding_(padding) {
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) noexcept {
                    return lhs.lng < rhs.lng;
                });
        min_lng_ = left_it->lng;
        const double max_lng = right_it->lng;

        const auto [bottom_it, top_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) noexcept {
                    return lhs.lat < rhs.lat;
                });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lng - min_lng_)) {
            width_zoom = (max_width - 2 * padding) / (max_lng - min_lng_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_factor_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_factor_ = *width_zoom;
        } else if (height_zoom) {
            zoom_factor_ = *height_zoom;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const;

private:
    double padding_;
    double min_lng_ = 0;
    double max_lat_ = 0;
    double zoom_factor_ = 0;
};

struct Settings {
    struct Offset {
        double dx, dy;
    };

    double width, height;
    double padding;
    double line_width;
    double stop_radius;
    double underlayer_width;
    int bus_label_font_size, stop_label_font_size;
    Offset bus_label_offset, stop_label_offset;
    svg::color::Color underlayer_color;
    std::vector<svg::color::Color> color_palette;
};

class MapRenderer final {
public:
    void Initialize(Settings settings);

    svg::Document Render(std::unordered_set<StopPtr>&& active_stops,
                         std::vector<BusPtr>&& sorted_buses) const;

private:
    std::optional<Settings> settings_;
};

} // namespace transport_catalogue::renderer