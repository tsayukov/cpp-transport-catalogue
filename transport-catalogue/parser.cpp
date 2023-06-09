#include "parser.h"

#include <stdexcept>

namespace transport_catalogue::query {

namespace from_cli {

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace str_view_handler;

Any Parse(std::string_view text) {
    SplitView split_view(text);
    const auto command = split_view.NextStrippedSubstrBefore(' ');
    if (command == "Stop"sv) {
        if (!split_view.CanSplit(':')) {
            return Parse<Tag::StopInfo>(split_view);
        }
        return Parse<Tag::StopCreation>(split_view);
    } else if (command == "Bus"sv) {
        if (!split_view.CanSplit(':')) {
            return Parse<Tag::BusInfo>(split_view);
        }
        return Parse<Tag::BusCreation>(split_view);
    }
    throw std::invalid_argument("No such command '"s + std::string(command) + "'"s);
}

template<>
Any Parse<Tag::StopInfo>(SplitView split_view) {
    const auto stop_name = split_view.NextStrippedSubstrBefore(':');
    return Any(Query<Tag::StopInfo>{ std::string(stop_name) });
}

template<>
Any Parse<Tag::StopCreation>(SplitView split_view) {
    using namespace std::string_view_literals;
    using namespace str_view_handler;

    const auto stop_name = split_view.NextStrippedSubstrBefore(':');

    Query<Tag::StopCreation> query;
    query.stop_name = std::string(stop_name);
    const geo::Degree latitude = std::stod(std::string(split_view.NextStrippedSubstrBefore(',')));
    const geo::Degree longitude = std::stod(std::string(split_view.NextStrippedSubstrBefore(',')));
    query.coordinates = geo::Coordinates(latitude, longitude);

    query.stop_distances_subquery.from_stop_name = std::string(stop_name);
    while (!split_view.Empty()) {
        const auto distance = std::stod(std::string(split_view.NextStrippedSubstrBefore("m to"sv)));
        const auto to_stop_name = split_view.NextStrippedSubstrBefore(',');
        query.stop_distances_subquery.distances_to_stops.emplace(std::string(to_stop_name), distance);
    }
    return Any(std::move(query));
}

template<>
Any Parse<Tag::BusInfo>(SplitView split_view) {
    const auto bus_name = split_view.NextStrippedSubstrBefore(':');
    return Any(Query<Tag::BusInfo>{ std::string(bus_name) });
}

template<>
Any Parse<Tag::BusCreation>(SplitView split_view) {
    const auto bus_name = split_view.NextStrippedSubstrBefore(':');
    Query<Tag::BusCreation> query;
    query.bus_name = std::string(bus_name);

    const auto [sep, _] = split_view.CanSplit('>', '-');
    query.route_type = (sep == '>') ? Bus::RouteType::Full : Bus::RouteType::Half;

    while (!split_view.Empty()) {
        const auto stop_name = split_view.NextStrippedSubstrBefore(sep);
        query.stops.emplace_back(stop_name);
    }
    return Any(std::move(query));
}

} // namespace from_cli

namespace from_json {

using namespace std::string_literals;

Parser::Result Parse(const json::Document& document) {
    return Parser(document).GetResult();
}

// Parser

Parser::Parser(const json::Document& document) noexcept
        : document_(document) {
}

Parser::Result&& Parser::GetResult() {
    const auto& root_map = document_.GetRoot().AsMap();
    if (auto iter = root_map.find("base_requests"s); iter != root_map.end()) {
        ParseBaseQueries(iter->second.AsArray());
    }
    if (auto iter = root_map.find("stat_requests"s); iter != root_map.end()) {
        ParseStatQueries(iter->second.AsArray());
    }
    if (auto iter = root_map.find("render_settings"s); iter != root_map.end()) {
        ParseRenderSettings(iter->second.AsMap());
    }
    return std::move(result_);
}

void Parser::ParseBaseQueries(const json::Array& array) {
    for (const json::Node& node : array) {
        const auto& query_map = node.AsMap();
        if (query_map.at("type"s) == "Stop"s) {
            ParseQuery(query_map, ParseVariant<Tag::StopCreation>{});
        } else if (query_map.at("type"s) == "Bus"s) {
            ParseQuery(query_map, ParseVariant<Tag::BusCreation>{});
        }
    }
}

void Parser::ParseStatQueries(const json::Array& array) {
    for (const json::Node& node : array) {
        const auto& query_map = node.AsMap();
        if (query_map.at("type"s) == "Stop"s) {
            ParseQuery(query_map, ParseVariant<Tag::StopInfo>{});
        } else if (query_map.at("type"s) == "Bus"s) {
            ParseQuery(query_map, ParseVariant<Tag::BusInfo>{});
        }
    }
}

void Parser::ParseRenderSettings(const json::Map& map) {
    auto& rs = result_.render_settings_.settings;

    auto check_attribute = [](const std::string& attribute, auto value, auto min, auto max) {
        if (value < min || value > max) {
            throw std::invalid_argument(
                    "render_settings."s + attribute
                    + " must be in ["s + std::to_string(min) + ", "s + std::to_string(max) + "]"s);
        }
    };

    rs.width = map.at("width"s).AsDouble();
    check_attribute("width"s, rs.width, 0.0, 100'000.0);

    rs.height = map.at("height"s).AsDouble();
    check_attribute("height"s, rs.height, 0.0, 100'000.0);

    rs.padding = map.at("padding"s).AsDouble();
    if (rs.padding < 0.0 || rs.padding >= std::max(rs.width, rs.height) / 2.0) {
        throw std::invalid_argument("render_settings.padding must be in [0.0, max(width, height) / 2)"s);
    }

    rs.line_width = map.at("line_width"s).AsDouble();
    check_attribute("line_width"s, rs.line_width, 0.0, 100'000.0);

    rs.stop_radius = map.at("stop_radius"s).AsDouble();
    check_attribute("stop_radius"s, rs.stop_radius, 0.0, 100'000.0);

    rs.bus_label_font_size = map.at("bus_label_font_size"s).AsInt();
    check_attribute("bus_label_font_size"s, rs.bus_label_font_size, 0, 100'000);

    const json::Array& bus_label_offset = map.at("bus_label_offset"s).AsArray();
    rs.bus_label_offset.dx = bus_label_offset[0].AsDouble();
    check_attribute("bus_label_offset.dx"s, rs.bus_label_offset.dx, -100'000.0, 100'000.0);
    rs.bus_label_offset.dy = bus_label_offset[1].AsDouble();
    check_attribute("bus_label_offset.dy"s, rs.bus_label_offset.dy, -100'000.0, 100'000.0);

    rs.stop_label_font_size = map.at("stop_label_font_size"s).AsInt();
    check_attribute("stop_label_font_size"s, rs.stop_label_font_size, 0, 100'000);

    const json::Array& stop_label_offset = map.at("stop_label_offset"s).AsArray();
    rs.stop_label_offset.dx = stop_label_offset[0].AsDouble();
    check_attribute("stop_label_offset.dx"s, rs.stop_label_offset.dx, -100'000.0, 100'000.0);
    rs.stop_label_offset.dy = stop_label_offset[1].AsDouble();
    check_attribute("stop_label_offset.dy"s, rs.stop_label_offset.dy, -100'000.0, 100'000.0);

    rs.underlayer_width = map.at("underlayer_width"s).AsDouble();
    check_attribute("underlayer_width"s, rs.underlayer_width, 0.0, 100'000.0);

    const auto& underlayer_color = map.at("underlayer_color"s);
    rs.underlayer_color = ParseColor(underlayer_color);

    const auto& color_palette = map.at("color_palette"s).AsArray();
    for (const auto& node_color : color_palette) {
        rs.color_palette.push_back(ParseColor(node_color));
    }
}

void Parser::ParseQuery(const json::Map& map, ParseVariant<Tag::StopInfo>) {
    result_.stat_queries_.emplace_back(map.at("id"s).AsInt(), Query<Tag::StopInfo>{map.at("name"s).AsString() });
}

void Parser::ParseQuery(const json::Map& map, ParseVariant<Tag::StopCreation>) {
    Query<Tag::StopCreation> query;
    query.stop_name = map.at("name"s).AsString();
    const geo::Degree latitude = map.at("latitude"s).AsDouble();
    const geo::Degree longitude = map.at("longitude"s).AsDouble();
    query.coordinates = geo::Coordinates(latitude, longitude);

    query.stop_distances_subquery.from_stop_name = std::string(query.stop_name);
    for (const auto& [to_stop_name, node] : map.at("road_distances"s).AsMap()) {
        const auto distance = node.AsDouble();
        query.stop_distances_subquery.distances_to_stops.emplace(to_stop_name, distance);
    }

    result_.base_queries_.emplace_back(std::move(query));
}

void Parser::ParseQuery(const json::Map& map, ParseVariant<Tag::BusInfo>) {
    result_.stat_queries_.emplace_back(map.at("id"s).AsInt(), Query<Tag::BusInfo>{map.at("name"s).AsString() });
}

void Parser::ParseQuery(const json::Map& map, ParseVariant<Tag::BusCreation>) {
    Query<Tag::BusCreation> query;
    query.bus_name = map.at("name"s).AsString();

    query.route_type = map.at("is_roundtrip"s).AsBool() ? Bus::RouteType::Full : Bus::RouteType::Half;

    for (const auto& node : map.at("stops"s).AsArray()) {
        const auto& stop_name = node.AsString();
        query.stops.emplace_back(stop_name);
    }

    result_.base_queries_.emplace_back(std::move(query));
}

svg::color::Color Parser::ParseColor(const json::Node& node) {
    if (node.IsString()) {
        return node.AsString();
    } else if (node.IsArray()) {
        const auto& color_array = node.AsArray();
        if (color_array.size() < 3) {
            throw std::invalid_argument("Unrecognized color format: expected RGB or RGBA format"s);
        }

        auto check_color_range = [](auto rgb_single_code) {
            if (rgb_single_code < 0 || rgb_single_code > 255) {
                throw std::invalid_argument("RGB code must be in [0, 255]"s);
            }
        };

        const auto red = color_array[0].AsInt();
        check_color_range(red);
        const auto green = color_array[1].AsInt();
        check_color_range(green);
        const auto blue = color_array[2].AsInt();
        check_color_range(blue);

        if (color_array.size() == 3) {
            return svg::color::Rgb{
                    static_cast<std::uint8_t>(red),
                    static_cast<std::uint8_t>(green),
                    static_cast<std::uint8_t>(blue)
            };
        } else if (color_array.size() == 4) {
            const double opacity = color_array[3].AsDouble();
            if (opacity < 0.0 || opacity > 1.0) {
                throw std::invalid_argument("Unrecognized color format: opacity (in RGBA) must be in [0.0, 1.0]"s);
            }

            return svg::color::Rgba{
                    static_cast<std::uint8_t>(red),
                    static_cast<std::uint8_t>(green),
                    static_cast<std::uint8_t>(blue),
                    opacity
            };
        }
        throw std::invalid_argument("Unrecognized color format: expected three (RGB) or four (RGBA) numbers"s);
    } else {
        throw std::invalid_argument("Unrecognized color format: expected string, RGB, or RGBA format"s);
    }
}

} // namespace from_json

} // namespace transport_catalogue::query