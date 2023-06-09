#include "map_renderer.h"

#include <iterator>

namespace transport_catalogue::renderer {

// SphereProjector

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return {(coords.lng - min_lng_) * zoom_factor_ + padding_,
            (max_lat_ - coords.lat) * zoom_factor_ + padding_ };
}

// MapRenderer

void MapRenderer::Initialize(Settings settings) {
    settings_ = std::move(settings);
}

svg::Document MapRenderer::Render(std::unordered_set<StopPtr>&& active_stops,
                                  std::vector<BusPtr>&& sorted_buses) const {
    using namespace std::string_literals;

    if (!settings_.has_value()) {
        throw std::runtime_error("MapRenderer must be initialized"s);
    }

    std::vector<geo::Coordinates> all_coordinates;
    all_coordinates.reserve(active_stops.size());
    std::transform(
            active_stops.cbegin(), active_stops.cend(),
            std::back_inserter(all_coordinates),
            [](StopPtr stop_ptr) noexcept {
                return stop_ptr->coordinates;
            });

    SphereProjector projector(
            all_coordinates.cbegin(), all_coordinates.cend(),
            settings_->width, settings_->height, settings_->padding);

    svg::Document document;

    auto color_iter = settings_->color_palette.cbegin();
    auto color_end = settings_->color_palette.cend();
    for (BusPtr bus_ptr : sorted_buses) {
        const auto& stops = bus_ptr->stops;
        auto route = svg::Polyline()
            .SetFillColor(svg::color::None)
            .SetStrokeWidth(settings_->line_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        if (color_iter == color_end) {
            color_iter = settings_->color_palette.cbegin();
        }
        route.SetStrokeColor(*color_iter);
        ++color_iter;

        for (StopPtr stop_ptr : stops) {
            route.AddPoint(projector(stop_ptr->coordinates));
        }

        document.Add(std::move(route));
    }

    return document;
}

} // namespace transport_catalogue::renderer