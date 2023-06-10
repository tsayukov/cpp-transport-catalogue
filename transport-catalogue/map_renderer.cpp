#include "map_renderer.h"

#include <iterator>

namespace transport_catalogue::renderer {

using namespace std::string_literals;

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

    templates_.route_
        .SetFillColor(svg::color::None)
        .SetStrokeWidth(settings_->line_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    templates_.underlayer_bus_name
        .SetOffset(settings_->bus_label_offset)
        .SetFontSize(settings_->bus_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFontWeight("bold"s);

    templates_.bus_name_ = templates_.underlayer_bus_name;

    templates_.underlayer_bus_name
        .SetFillColor(settings_->underlayer_color)
        .SetStrokeColor(settings_->underlayer_color)
        .SetStrokeWidth(settings_->underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    templates_.stop_
        .SetRadius(settings_->stop_radius)
        .SetFillColor("white"s);

    templates_.underlayer_stop_name_
        .SetOffset(settings_->stop_label_offset)
        .SetFontSize(settings_->stop_label_font_size)
        .SetFontFamily("Verdana"s);

    templates_.stop_name_ = templates_.underlayer_stop_name_;
    templates_.stop_name_
        .SetFillColor("black"s);

    templates_.underlayer_stop_name_
        .SetFillColor(settings_->underlayer_color)
        .SetStrokeColor(settings_->underlayer_color)
        .SetStrokeWidth(settings_->underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

SphereProjector MapRenderer::MakeProjector(const std::vector<StopPtr>& sorted_active_stops) const {
    std::vector<geo::Coordinates> all_coordinates;
    all_coordinates.reserve(sorted_active_stops.size());
    std::transform(
            sorted_active_stops.cbegin(), sorted_active_stops.cend(),
            std::back_inserter(all_coordinates),
            [](StopPtr stop_ptr) noexcept {
                return stop_ptr->coordinates;
            });

    return { all_coordinates.cbegin(), all_coordinates.cend(),
             settings_->width, settings_->height, settings_->padding };
}

void MapRenderer::RenderRoutes(svg::Document& document, const SphereProjector& projector,
                               const std::vector<BusPtr>& sorted_buses) const {
    auto color_iter = settings_->color_palette.cbegin();
    auto color_end = settings_->color_palette.cend();

    for (BusPtr bus_ptr : sorted_buses) {
        if (bus_ptr->stops.empty()) {
            continue;
        }

        const auto& stops = bus_ptr->stops;
        auto route = templates_.route_;

        if (color_iter == color_end) {
            color_iter = settings_->color_palette.cbegin();
        }
        route.SetStrokeColor(*color_iter);

        for (StopPtr stop_ptr : stops) {
            route.AddPoint(projector(stop_ptr->coordinates));
        }
        if (bus_ptr->route_type == Bus::RouteType::Half) {
            for (auto iter = ++stops.crbegin(); iter != stops.crend(); ++iter) {
                route.AddPoint(projector((*iter)->coordinates));
            }
        }

        document.Add(std::move(route));

        ++color_iter;
    }
}

void MapRenderer::RenderBusNames(svg::Document& document, const SphereProjector& projector,
                                 const std::vector<BusPtr>& sorted_buses) const {
    auto color_iter = settings_->color_palette.cbegin();
    auto color_end = settings_->color_palette.cend();

    for (BusPtr bus_ptr : sorted_buses) {
        if (bus_ptr->stops.empty()) {
            continue;
        }

        const auto& stops = bus_ptr->stops;

        if (color_iter == color_end) {
            color_iter = settings_->color_palette.cbegin();
        }


        document.Add(svg::Text(templates_.underlayer_bus_name)
                .SetData(bus_ptr->name)
                .SetPosition(projector(stops.front()->coordinates)));

        document.Add(svg::Text(templates_.bus_name_)
                .SetData(bus_ptr->name)
                .SetPosition(projector(stops.front()->coordinates))
                .SetFillColor(*color_iter));

        if (bus_ptr->route_type == Bus::RouteType::Half && stops.front() != stops.back()) {
            document.Add(svg::Text(templates_.underlayer_bus_name)
                    .SetData(bus_ptr->name)
                    .SetPosition(projector(stops.back()->coordinates)));

            document.Add(svg::Text(templates_.bus_name_)
                    .SetData(bus_ptr->name)
                    .SetPosition(projector(stops.back()->coordinates))
                    .SetFillColor(*color_iter));
        }

        ++color_iter;
    }
}

void MapRenderer::RenderStops(svg::Document& document, const SphereProjector& projector,
                              const std::vector<StopPtr>& sorted_active_stops) const {
    for (StopPtr stop_ptr : sorted_active_stops) {
        document.Add(svg::Circle(templates_.stop_)
                .SetCenter(projector(stop_ptr->coordinates)));
    }
}

void MapRenderer::RenderStopNames(svg::Document& document, const SphereProjector& projector,
                                  const std::vector<StopPtr>& sorted_active_stops) const {
    for (StopPtr stop_ptr : sorted_active_stops) {
        document.Add(svg::Text(templates_.underlayer_stop_name_)
                .SetPosition(projector(stop_ptr->coordinates))
                .SetData(stop_ptr->name));
        document.Add(svg::Text(templates_.stop_name_)
                 .SetPosition(projector(stop_ptr->coordinates))
                 .SetData(stop_ptr->name));
    }
}

} // namespace transport_catalogue::renderer