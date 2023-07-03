/// \file
/// Rendering transport routes into SVG

#pragma once

#include "geo.h"
#include "domain.h"
#include "svg.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <numeric>
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
        const geo::Degree max_lng = right_it->lng;

        const auto [bottom_it, top_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) noexcept {
                    return lhs.lat < rhs.lat;
                });
        const geo::Degree min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero((max_lng - min_lng_).Get())) {
            width_zoom = (max_width - 2 * padding) / (max_lng - min_lng_).Get();
        }

        std::optional<double> height_zoom;
        if (!IsZero((max_lat_ - min_lat).Get())) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat).Get();
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
    geo::Degree padding_;
    geo::Degree min_lng_;
    geo::Degree max_lat_;
    double zoom_factor_ = 0;
};

struct Settings {
    double width, height;
    double padding;
    double line_width;
    double stop_radius;
    double underlayer_width;
    int bus_label_font_size, stop_label_font_size;
    svg::Point bus_label_offset, stop_label_offset;
    svg::color::Color underlayer_color;
    std::vector<svg::color::Color> color_palette;
};

struct Templates {
    svg::Polyline route_;

    svg::Text underlayer_bus_name;
    svg::Text bus_name_;

    svg::Circle stop_;

    svg::Text underlayer_stop_name_;
    svg::Text stop_name_;
};

class MapRenderer final {
public:
    void Initialize(Settings settings);

    template<typename BusesIter>
    [[nodiscard]] svg::Document Render(BusesIter bus_first, BusesIter bus_last) const {
        if (!settings_.has_value()) {
            using namespace std::string_literals;
            throw std::runtime_error("MapRenderer must be initialized"s);
        }

        const std::vector<BusPtr> sorted_buses = std::invoke([bus_first, bus_last] {
            std::vector<BusPtr> buses;
            buses.reserve(std::distance(bus_first, bus_last));
            std::transform(
                    bus_first, bus_last,
                    std::back_inserter(buses),
                    [](const Bus& bus) noexcept {
                        return &bus;
                    });
            std::sort(
                    buses.begin(), buses.end(),
                    [](BusPtr lhs, BusPtr rhs) noexcept {
                        return lhs->name < rhs->name;
                    });

            return buses;
        });

        const std::vector<StopPtr> sorted_active_stops = std::invoke([&sorted_buses] {
            const std::size_t size = std::accumulate(
                    sorted_buses.cbegin(), sorted_buses.cend(),
                    std::size_t(0),
                    [](std::size_t acc, BusPtr bus_ptr) {
                        return acc + bus_ptr->stops.size();
                    });

            std::vector<StopPtr> stops;
            stops.reserve(size);
            for (BusPtr bus_ptr : sorted_buses) {
                for (StopPtr stop_ptr : bus_ptr->stops) {
                    stops.push_back(stop_ptr);
                }
            }

            std::sort(
                    stops.begin(), stops.end(),
                    [](StopPtr lhs, StopPtr rhs) {
                        return lhs->name < rhs->name;
                    });
            auto begin_to_remove = std::unique(
                    stops.begin(), stops.end(),
                    [](StopPtr lhs, StopPtr rhs) {
                        return lhs->name == rhs->name;
                    });
            stops.erase(begin_to_remove, stops.end());

            return stops;
        });

        SphereProjector projector = MakeProjector(sorted_active_stops);

        svg::Document document;

        RenderRoutes(document, projector, sorted_buses);
        RenderBusNames(document, projector, sorted_buses);
        RenderStops(document, projector, sorted_active_stops);
        RenderStopNames(document, projector, sorted_active_stops);

        return document;
    }

private:
    std::optional<Settings> settings_;
    Templates templates_;

    SphereProjector MakeProjector(const std::vector<StopPtr>& sorted_active_stops) const;

    void RenderRoutes(svg::Document& document, const SphereProjector& projector,
                      const std::vector<BusPtr>& sorted_buses) const;
    void RenderBusNames(svg::Document& document, const SphereProjector& projector,
                        const std::vector<BusPtr>& sorted_buses) const;
    void RenderStops(svg::Document& document, const SphereProjector& projector,
                     const std::vector<StopPtr>& sorted_active_stops) const;
    void RenderStopNames(svg::Document& document, const SphereProjector& projector,
                         const std::vector<StopPtr>& sorted_active_stops) const;
};

} // namespace transport_catalogue::renderer