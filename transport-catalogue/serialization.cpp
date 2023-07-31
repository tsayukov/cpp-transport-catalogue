#include "serialization.h"

#include "domain.h"
#include "json_reader.h"
#include "svg.h"
#include "graph.h"
#include "router.h"
#include "ranges.h"

#include <transport_catalogue.pb.h>
#include <map_renderer.pb.h>
#include <svg.pb.h>
#include <transport_router.pb.h>
#include <graph.pb.h>

#include <fstream>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace transport_catalogue::serialization {

using namespace std::string_literals;

namespace db_proto = transport_catalogue_proto;
namespace map_proto = map_renderer_proto;
namespace router_proto = transport_router_proto;

void Serializer::Initialize(Settings settings) {
    settings_ = std::move(settings);
}

[[nodiscard]] db_proto::Database GetProtoDatabase(const TransportCatalogue& database) {
    db_proto::Database proto_database;

    std::unordered_map<StopPtr, std::int32_t> stop_ptr_to_index;
    {
        std::int32_t index = 0;
        for (const auto& stop : database.GetAllStops()) {
            db_proto::Stop proto_stop;
            proto_stop.set_name(stop.name);

            db_proto::Coordinates proto_coordinates;
            proto_coordinates.set_latitude(stop.coordinates.lat.Get());
            proto_coordinates.set_longitude(stop.coordinates.lng.Get());
            *proto_stop.mutable_coordinates() = std::move(proto_coordinates);

            *proto_database.add_stops() = std::move(proto_stop);

            stop_ptr_to_index.emplace(&stop, index);
            index += 1;
        }
    }
    for (const auto& bus : database.GetAllBuses()) {
        db_proto::Bus proto_bus;
        proto_bus.set_name(bus.name);

        db_proto::RouteType proto_route_type = (bus.route_type == Bus::RouteType::Half)
                                               ? db_proto::RouteType::Half
                                               : db_proto::RouteType::Full;
        proto_bus.set_route_type(proto_route_type);

        for (StopPtr stop_ptr : bus.stops) {
            proto_bus.add_stop_indices(stop_ptr_to_index.at(stop_ptr));
        }

        *proto_database.add_buses() = std::move(proto_bus);
    }

    for (const auto& [stop_ptr_pair, distance] : database.GetDistances()) {
        db_proto::Distance proto_distance;
        proto_distance.set_from_stop_index(stop_ptr_to_index.at(stop_ptr_pair.first));
        proto_distance.set_to_stop_index(stop_ptr_to_index.at(stop_ptr_pair.second));
        proto_distance.set_distance(distance.Get());

        *proto_database.add_distances() = std::move(proto_distance);
    }

    return proto_database;
}

template<typename ColorVariantType>
[[nodiscard]] svg_proto::Color GetProtoColor(ColorVariantType&& value) {
    using value_type = std::decay_t<decltype(value)>;

    svg_proto::Color proto_color;
    if constexpr (std::is_same_v<value_type, std::monostate>) {
        proto_color.set_name("None"s);
    } else if constexpr (std::is_same_v<value_type, std::string>) {
        proto_color.set_name(value);
    } else if constexpr (std::is_same_v<value_type, svg::color::Rgb>) {
        svg_proto::Rgb& proto_rgb = *proto_color.mutable_rgb();
        proto_rgb.set_red(value.red);
        proto_rgb.set_green(value.green);
        proto_rgb.set_blue(value.blue);
    } else if constexpr (std::is_same_v<value_type, svg::color::Rgba>) {
        svg_proto::Rgba& proto_rgba = *proto_color.mutable_rgba();
        proto_rgba.set_red(value.red);
        proto_rgba.set_green(value.green);
        proto_rgba.set_blue(value.blue);
        proto_rgba.set_opacity(value.opacity);
    }
    return proto_color;
}

[[nodiscard]] map_proto::Settings GetProtoMapRendererSettings(const renderer::Settings& settings) {
    map_proto::Settings proto_settings;

    proto_settings.set_width(settings.width);
    proto_settings.set_height(settings.height);
    proto_settings.set_padding(settings.padding);
    proto_settings.set_line_width(settings.line_width);
    proto_settings.set_stop_radius(settings.stop_radius);
    proto_settings.set_underlayer_width(settings.underlayer_width);
    proto_settings.set_bus_label_font_size(settings.bus_label_font_size);
    proto_settings.set_stop_label_font_size(settings.stop_label_font_size);

    {
        svg_proto::Point proto_point;
        proto_point.set_x(settings.bus_label_offset.x);
        proto_point.set_y(settings.bus_label_offset.y);
        *proto_settings.mutable_bus_label_offset() = proto_point;
    }
    {
        svg_proto::Point proto_point;
        proto_point.set_x(settings.stop_label_offset.x);
        proto_point.set_y(settings.stop_label_offset.y);
        *proto_settings.mutable_stop_label_offset() = proto_point;
    }

    const auto get_proto_color = [](auto&& value) { return GetProtoColor(value); };

    *proto_settings.mutable_underlayer_color() = std::visit(get_proto_color, settings.underlayer_color);
    for (const auto& color : settings.color_palette) {
        *proto_settings.add_color_palette() = std::visit(get_proto_color, color);
    }

    return proto_settings;
}

[[nodiscard]] router_proto::Settings GetProtoRouterSettings(router::Settings settings) {
    router_proto::Settings proto_settings;
    proto_settings.set_bus_wait_time(settings.bus_wait_time.Get());
    proto_settings.set_bus_velocity(settings.bus_velocity.Get());
    return proto_settings;
}

[[nodiscard]] graph_proto::Weight GetProtoWeight(const router::TransportRouter::Item& item) {
    graph_proto::Weight proto_weight;

    if (item.IsWaitItem()) {
        graph_proto::WaitItem proto_wait_item;
        proto_wait_item.set_time(item.GetTime().Get());
        *proto_weight.mutable_wait_item() = proto_wait_item;
    } else if (item.IsBusItem()) {
        graph_proto::BusItem proto_bus_item;
        const auto& bus_item = item.GetBusItem();
        proto_bus_item.set_time(bus_item.time.Get());
        proto_bus_item.set_span_count(bus_item.span_count);
        *proto_weight.mutable_bus_item() = proto_bus_item;
    } else {
        graph_proto::CombineItem proto_combine_item;
        proto_combine_item.set_time(item.GetTime().Get());
        *proto_weight.mutable_combine_item() = proto_combine_item;
    }

    return proto_weight;
}

[[nodiscard]] graph_proto::Graph GetProtoGraph(const graph::DirectedWeightedGraph<router::TransportRouter::Item>& graph) {
    graph_proto::Graph proto_graph;

    std::vector<const graph::Edge<router::TransportRouter::Item>*> edge_ptrs(graph.GetEdgeCount(), nullptr);
    for (const auto vertex_id : ranges::Indices(graph.GetVertexCount())) {
        auto& proto_incidence_list = *proto_graph.add_incidence_lists();
        for (const auto edge_id : graph.GetIncidentEdges(vertex_id)) {
            proto_incidence_list.add_edge_id(edge_id);
            edge_ptrs[edge_id] = &graph.GetEdge(edge_id);
        }
    }

    for (const auto edge_ptr : edge_ptrs) {
        graph_proto::Edge proto_edge;
        proto_edge.set_from(edge_ptr->from);
        proto_edge.set_to(edge_ptr->to);
        *proto_edge.mutable_weight() = GetProtoWeight(edge_ptr->weight);
        *proto_graph.add_edges() = std::move(proto_edge);
    }

    return proto_graph;
}

[[nodiscard]] router_proto::Router GetProtoRouter(const graph::Router<router::TransportRouter::Item>& router) {
    router_proto::Router proto_router;

    for (const auto& row : router.GetRoutesInternalData()) {
        router_proto::RowRoutesInternalData proto_row;

        for (const auto& internal_data : row) {
            router_proto::RouteInternalData proto_internal_data;
            if (internal_data.has_value()) {
                *proto_internal_data.mutable_weight() = GetProtoWeight(internal_data->weight);
                if (internal_data->prev_edge.has_value()) {
                    router_proto::PreviousEdge proto_prev_edge;
                    proto_prev_edge.set_edge_id(internal_data->prev_edge.value());
                    *proto_internal_data.mutable_previous_edge() = proto_prev_edge;
                }
            }
            *proto_row.add_row() = std::move(proto_internal_data);
        }

        *proto_router.add_routes_internal_data() = std::move(proto_row);
    }

    return proto_router;
}

void Serializer::Serialize(SentData sent_data) const {
    if (!settings_.has_value()) {
        throw std::logic_error("Settings must be initialized before serialization"s);
    }

    std::ofstream output(settings_->file, std::ios::binary);

    db_proto::TransportCatalogue proto_catalogue;
    *proto_catalogue.mutable_database() = GetProtoDatabase(sent_data.database);
    if (sent_data.render_settings.has_value()) {
        *proto_catalogue.mutable_render_settings() = GetProtoMapRendererSettings(sent_data.render_settings.value());
    }

    if (const auto router_settings = sent_data.transport_router.GetSettings()) {
        router_proto::TransportRouter proto_transport_router;
        *proto_transport_router.mutable_settings() = GetProtoRouterSettings(router_settings.value());
        *proto_transport_router.mutable_graph() = GetProtoGraph(sent_data.transport_router.GetGraph());
        *proto_transport_router.mutable_router() = GetProtoRouter(sent_data.transport_router.GetRouter().value());
        *proto_catalogue.mutable_transport_router() = std::move(proto_transport_router);
    }

    proto_catalogue.SerializeToOstream(&output);
}

[[nodiscard]] TransportCatalogue GetDatabase(const db_proto::Database& proto_database) {
    TransportCatalogue database;

    for (auto& proto_stop : proto_database.stops()) {
        Stop stop;
        stop.name = proto_stop.name();
        stop.coordinates.lat = geo::Degree{proto_stop.coordinates().latitude()};
        stop.coordinates.lng = geo::Degree{proto_stop.coordinates().longitude()};

        database.AddStop(std::move(stop));
    }

    {
        std::vector<std::string_view> stop_names;
        for (auto& proto_bus : proto_database.buses()) {
            stop_names.reserve(proto_bus.stop_indices_size());
            for (const auto index : proto_bus.stop_indices()) {
                stop_names.emplace_back(proto_database.stops().at(index).name());
            }

            Bus::RouteType route_type = (proto_bus.route_type() == db_proto::RouteType::Half)
                                        ? Bus::RouteType::Half
                                        : Bus::RouteType::Full;

            database.AddBus(proto_bus.name(), stop_names, route_type);

            stop_names.clear();
        }
    }

    for (const auto& distance : proto_database.distances()) {
        const std::string_view from_stop_name = proto_database.stops().at(distance.from_stop_index()).name();
        const std::string_view to_stop_name = proto_database.stops().at(distance.to_stop_index()).name();
        const auto distance_m = geo::Meter{distance.distance()};
        database.SetDistanceBetweenStops(from_stop_name, to_stop_name, distance_m);
    }

    return database;
}

[[nodiscard]] svg::color::Color GetSvgColor(const svg_proto::Color& proto_color) {
    using ColorCase = svg_proto::Color::ColorCase;

    switch (proto_color.color_case()) {
        case ColorCase::COLOR_NOT_SET: {
            return svg::color::None;
        }
        case ColorCase::kName: {
            return proto_color.name();
        }
        case ColorCase::kRgb: {
            svg::color::Rgb rgb;
            rgb.red = proto_color.rgb().red();
            rgb.green = proto_color.rgb().green();
            rgb.blue = proto_color.rgb().blue();
            return rgb;
        }
        case ColorCase::kRgba: {
            svg::color::Rgba rgba;
            rgba.red = proto_color.rgba().red();
            rgba.green = proto_color.rgba().green();
            rgba.blue = proto_color.rgba().blue();
            rgba.opacity = proto_color.rgba().opacity();
            return rgba;
        }
    }
}

[[nodiscard]] renderer::Settings GetMapRendererSettings(const map_proto::Settings& proto_settings) {
    renderer::Settings settings;

    settings.width = proto_settings.width();
    settings.height = proto_settings.height();
    settings.padding = proto_settings.padding();
    settings.line_width = proto_settings.line_width();
    settings.stop_radius = proto_settings.stop_radius();
    settings.underlayer_width = proto_settings.underlayer_width();
    settings.bus_label_font_size = proto_settings.bus_label_font_size();
    settings.stop_label_font_size = proto_settings.stop_label_font_size();

    {
        svg::Point point;
        point.x = proto_settings.bus_label_offset().x();
        point.y = proto_settings.bus_label_offset().y();
        settings.bus_label_offset = point;
    }
    {
        svg::Point point;
        point.x = proto_settings.stop_label_offset().x();
        point.y = proto_settings.stop_label_offset().y();
        settings.stop_label_offset = point;
    }

    settings.underlayer_color = GetSvgColor(proto_settings.underlayer_color());

    settings.color_palette.reserve(proto_settings.color_palette_size());
    for (const auto& proto_color : proto_settings.color_palette()) {
        settings.color_palette.emplace_back(GetSvgColor(proto_color));
    }

    return settings;
}

[[nodiscard]] router::TransportRouter::Item GetItem(const graph_proto::Weight& proto_weight) {
    router::TransportRouter::Item item;
    switch (proto_weight.weight_case()) {
        case graph_proto::Weight::kCombineItem: {
            const auto time = router::Minute{proto_weight.combine_item().time()};
            item = router::TransportRouter::CombineItem{time};
            break;
        }
        case graph_proto::Weight::kWaitItem: {
            const auto time = router::Minute{proto_weight.wait_item().time()};
            item = router::TransportRouter::WaitItem{time};
            break;
        }
        case graph_proto::Weight::kBusItem: {
            const auto time = router::Minute{proto_weight.bus_item().time()};
            const auto span_count = proto_weight.bus_item().span_count();
            item = router::TransportRouter::BusItem{time, span_count};
            break;
        }
        case graph_proto::Weight::WEIGHT_NOT_SET: {
            throw std::logic_error("Edge's weight must be either Combine, or Wait, or Bus item"s);
        }
    }
    return item;
}

[[nodiscard]] router::Settings GetRouterSettings(const router_proto::Settings& proto_settings) {
    router::Settings settings;
    settings.bus_wait_time = router::Minute{proto_settings.bus_wait_time()};
    settings.bus_velocity = router::KmPerHour{proto_settings.bus_velocity()};
    return settings;
}

[[nodiscard]] auto GetGraph(const graph_proto::Graph& proto_graph) {
    graph::DirectedWeightedGraph<router::TransportRouter::Item> graph(proto_graph.incidence_lists_size());
    for (const auto& proto_edge : proto_graph.edges()) {
        graph.AddEdge({proto_edge.from(), proto_edge.to(), GetItem(proto_edge.weight())});
    }
    return graph;
}

[[nodiscard]] auto GetRoutesInternalData(const router_proto::Router& proto_router) {
    using RouteInternalData = graph::Router<router::TransportRouter::Item>::RouteInternalData;
    using RoutesInternalData = graph::Router<router::TransportRouter::Item>::RoutesInternalData;

    RoutesInternalData routes_internal_data;

    routes_internal_data.reserve(proto_router.routes_internal_data_size());
    for (const auto& proto_row : proto_router.routes_internal_data()) {
        auto& row = routes_internal_data.emplace_back();
        row.reserve(proto_row.row_size());

        for (const auto& proto_route_internal_data : proto_row.row()) {
            std::optional<RouteInternalData> route_internal_data;
            if (proto_route_internal_data.has_weight()) {

                route_internal_data.emplace(RouteInternalData{GetItem(proto_route_internal_data.weight()), std::nullopt});
                if (proto_route_internal_data.has_previous_edge()) {
                    route_internal_data->prev_edge = proto_route_internal_data.previous_edge().edge_id();
                }
            }
            row.emplace_back(route_internal_data);
        }
    }

    return routes_internal_data;
}

Serializer::ReceivedData Serializer::Deserialize() const {
    if (!settings_.has_value()) {
        throw std::logic_error("Settings must be initialized before deserialization"s);
    }

    std::ifstream input(settings_->file, std::ios::binary);

    db_proto::TransportCatalogue proto_catalogue;
    proto_catalogue.ParseFromIstream(&input);

    ReceivedData received_data{GetDatabase(proto_catalogue.database())};

    if (proto_catalogue.has_render_settings()) {
        received_data.render_settings = GetMapRendererSettings(*proto_catalogue.mutable_render_settings());
    }

    if (proto_catalogue.has_transport_router()) {
        router::TransportRouter transport_router;

        auto router_settings = GetRouterSettings(proto_catalogue.transport_router().settings());
        transport_router.Initialize(router_settings);
        transport_router.InitializeRouter(
                received_data.database,
                GetGraph(proto_catalogue.transport_router().graph()),
                GetRoutesInternalData(proto_catalogue.transport_router().router()));

        received_data.transport_router.emplace(std::move(transport_router));
    }

    return received_data;
}

} // namespace transport_catalogue::serialization