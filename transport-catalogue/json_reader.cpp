#include "json_reader.h"

#include "geo.h"
#include "domain.h"
#include "svg.h"
#include "json.h"
#include "json_builder.h"
#include "map_renderer.h"

#include <algorithm>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace transport_catalogue {

namespace from {

using namespace std::string_literals;
using namespace std::string_view_literals;

class JsonParser : public Parser {
public:
    void Parse(const json::Node& node, std::string_view request_type) {
        query_type = {};
        node_ptr_ = &node;

        if (request_type == "base_requests"sv) {
            std::string_view type = node_ptr_->AsDict().at("type"s).AsString();
            if (type == "Stop"sv) {
                query_type = "stop_creation"sv;
            } else if (type == "Bus"sv) {
                query_type = "bus_creation"sv;
            }
        } else if (request_type == "stat_requests"s) {
            std::string_view type = node_ptr_->AsDict().at("type"s).AsString();
            if (type == "Stop"sv) {
                query_type = "stop_info"sv;
            } else if (type == "Bus"sv) {
                query_type = "bus_info"sv;
            } else if (type == "Map"sv) {
                query_type = "renderer"sv;
            } else if (type == "RouteInfo"sv) {
                query_type = "router"sv;
            }
        } else if (request_type == "render_settings"s) {
            query_type = "renderer_setup"sv;
        } else if (request_type == "routing_settings"s) {
            query_type = "router_setup"sv;
        }
    }

    [[nodiscard]] QueryType GetQueryType() const override {
        return query_type;
    }

private:
    QueryType query_type;
    const json::Node* node_ptr_ = nullptr;

    [[nodiscard]] std::any GetObject(std::string_view object_name) const override {
        const auto& dict = node_ptr_->AsDict();
        if (object_name == "name"sv) {
            return dict.at("name"s).AsString();
        } else if (object_name == "id"sv) {
            return dict.at("id"s).AsInt();
        } else if (object_name == "coordinates"sv) {
            const auto latitude = geo::Degree{dict.at("latitude"s).AsDouble()};
            const auto longitude = geo::Degree{dict.at("longitude"s).AsDouble()};
            return geo::Coordinates(latitude, longitude);
        } else if (object_name == "distances"sv) {
            const auto& map = dict.at("road_distances"s).AsDict();
            std::unordered_map<std::string, geo::Meter> distances;
            distances.reserve(map.size());
            for (const auto& [to_stop_name, node] : map) {
                const auto distance = geo::Meter{node.AsDouble()};
                distances.emplace(to_stop_name, distance);
            }
            return distances;
        } else if (object_name == "stop_names"sv) {
            const auto& array = dict.at("stops"s).AsArray();
            std::vector<std::string> stop_names;
            stop_names.reserve(array.size());
            for (const auto& node : array) {
                const auto& stop_name = node.AsString();
                stop_names.emplace_back(stop_name);
            }
            return stop_names;
        } else if (object_name == "route_type"sv) {
            return dict.at("is_roundtrip"s).AsBool() ? Bus::RouteType::Full : Bus::RouteType::Half;
        } else if (object_name == "renderer_settings"sv) {
            return GetRendererSettings(dict);
        } else if (object_name == "router_settings"sv) {
            // todo impl
        }
        throw std::invalid_argument("No such type '"s + std::string(object_name) + "'"s);
    }

    static renderer::Settings GetRendererSettings(const json::Dict& dict) {
        renderer::Settings rs;

        auto check_attribute = [](const std::string& attribute, auto value, auto min, auto max) {
            if (value < min || value > max) {
                throw std::invalid_argument(
                        "render_settings."s + attribute
                        + " must be in ["s + std::to_string(min) + ", "s + std::to_string(max) + "]"s);
            }
        };

        rs.width = dict.at("width"s).AsDouble();
        check_attribute("width"s, rs.width, 0.0, 100'000.0);

        rs.height = dict.at("height"s).AsDouble();
        check_attribute("height"s, rs.height, 0.0, 100'000.0);

        rs.padding = dict.at("padding"s).AsDouble();
        if (rs.padding < 0.0 || rs.padding >= std::max(rs.width, rs.height) / 2.0) {
            throw std::invalid_argument("render_settings.padding must be in [0.0, max(width, height) / 2)"s);
        }

        rs.line_width = dict.at("line_width"s).AsDouble();
        check_attribute("line_width"s, rs.line_width, 0.0, 100'000.0);

        rs.stop_radius = dict.at("stop_radius"s).AsDouble();
        check_attribute("stop_radius"s, rs.stop_radius, 0.0, 100'000.0);

        rs.bus_label_font_size = dict.at("bus_label_font_size"s).AsInt();
        check_attribute("bus_label_font_size"s, rs.bus_label_font_size, 0, 100'000);

        const json::Array& bus_label_offset = dict.at("bus_label_offset"s).AsArray();
        rs.bus_label_offset.x = bus_label_offset[0].AsDouble();
        check_attribute("bus_label_offset.x"s, rs.bus_label_offset.x, -100'000.0, 100'000.0);
        rs.bus_label_offset.y = bus_label_offset[1].AsDouble();
        check_attribute("bus_label_offset.y"s, rs.bus_label_offset.y, -100'000.0, 100'000.0);

        rs.stop_label_font_size = dict.at("stop_label_font_size"s).AsInt();
        check_attribute("stop_label_font_size"s, rs.stop_label_font_size, 0, 100'000);

        const json::Array& stop_label_offset = dict.at("stop_label_offset"s).AsArray();
        rs.stop_label_offset.x = stop_label_offset[0].AsDouble();
        check_attribute("stop_label_offset.x"s, rs.stop_label_offset.x, -100'000.0, 100'000.0);
        rs.stop_label_offset.y = stop_label_offset[1].AsDouble();
        check_attribute("stop_label_offset.y"s, rs.stop_label_offset.y, -100'000.0, 100'000.0);

        rs.underlayer_width = dict.at("underlayer_width"s).AsDouble();
        check_attribute("underlayer_width"s, rs.underlayer_width, 0.0, 100'000.0);

        const auto& underlayer_color = dict.at("underlayer_color"s);
        rs.underlayer_color = ParseColor(underlayer_color);

        const auto& color_palette = dict.at("color_palette"s).AsArray();
        for (const auto& node_color : color_palette) {
            rs.color_palette.push_back(ParseColor(node_color));
        }

        return rs;
    }

    static svg::color::Color ParseColor(const json::Node& node) {
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
};

[[nodiscard]] ParseResult ReadQueries(from::Json, std::istream& input) {
    JsonParser parser;
    ParseResult result;
    const json::Document document = json::Load(input);
    for (const auto& [request_type, nodes] : document.GetRoot().AsDict()) {
        if (nodes.IsArray()) {
            for (const auto& node : nodes.AsArray()) {
                parser.Parse(node, request_type);
                const auto query_type = parser.GetQueryType();
                const auto& factory = queries::QueryFactory::GetFactory(query_type);
                result.PushBack(factory.Construct(parser), parser.GetQueryType());
            }
        } else if (nodes.IsDict()) {
            parser.Parse(nodes, request_type);
            const auto query_type = parser.GetQueryType();
            const auto& factory = queries::QueryFactory::GetFactory(query_type);
            result.PushBack(factory.Construct(parser), parser.GetQueryType());
        }
    }
    return result;
}

} // namespace from

namespace into {

using namespace std::string_literals;

using StopInfo = decltype(std::declval<queries::Handler>().GetStopInfo(std::declval<std::string_view>()));
using BusInfo = decltype(std::declval<queries::Handler>().GetBusInfo(std::declval<std::string_view>()));

json::Node StopInfoAsJson(int id, StopInfo stop_info) {
    auto builder = json::Builder{};
    auto dict_builder = builder
            .StartDict()
                .Key("request_id"s).Value(id);

    if (!stop_info.has_value()) {
        return dict_builder
                    .Key("error_message"s).Value("not found"s)
                .EndDict()
                .Build();
    }

    const auto& buses = stop_info.value()->buses;
    json::Array array;
    array.reserve(buses.size());
    std::transform(
            buses.cbegin(), buses.cend(),
            std::back_inserter(array),
            [](BusPtr bus_ptr) {
                return bus_ptr->name;
            });

    return dict_builder
                .Key("buses"s).Value(std::move(array))
            .EndDict()
            .Build();
}

json::Node BusInfoAsJson(int id, BusInfo bus_info) {
    auto builder = json::Builder{};
    auto dict_builder = builder
            .StartDict()
                .Key("request_id"s).Value(id);

    if (!bus_info.has_value()) {
        return dict_builder
                    .Key("error_message"s).Value("not found"s)
                .EndDict()
                .Build();
    }

    auto bus_stats_ptr = bus_info.value();
    return dict_builder
                .Key("stop_count"s).Value(static_cast<int>(bus_stats_ptr->stops_count))
                .Key("unique_stop_count"s).Value(static_cast<int>(bus_stats_ptr->unique_stops_count))
                .Key("route_length"s).Value(bus_stats_ptr->length.route.Get())
                .Key("curvature"s).Value(bus_stats_ptr->length.route / bus_stats_ptr->length.geo)
            .EndDict()
            .Build();
}

json::Node SvgAsJson(int id, const svg::Document& document) {
    std::ostringstream str_output;
    document.Render(str_output);

    return json::Builder{}
        .StartDict()
            .Key("request_id"s).Value(id)
            .Key("map"s).Value(str_output.str())
        .EndDict()
        .Build();
}

class JsonPrintDriver : public PrintDriver {
public:
    explicit JsonPrintDriver(std::ostream& output)
            : PrintDriver(output) {
        Initialize();
    }

    void Flush() const override {
        json::Print(json::Document(json::Node(std::move(array_))), output);
    }

    void Initialize() {
        using PrintableStopInfo = std::tuple<int, std::string_view, StopInfo>;
        using PrintableBusInfo = std::tuple<int, std::string_view, BusInfo>;
        using PrintableMap = std::tuple<int, svg::Document>;

        RegisterPrintOperation<PrintableStopInfo>([this](std::ostream&, const void* object) {
            const auto [id, _, stop_info] = *reinterpret_cast<const PrintableStopInfo*>(object);
            array_.emplace_back(StopInfoAsJson(id, stop_info));
        });
        RegisterPrintOperation<PrintableBusInfo>([this](std::ostream&, const void* object) {
            const auto [id, _, bus_info] = *reinterpret_cast<const PrintableBusInfo*>(object);
            array_.emplace_back(BusInfoAsJson(id, bus_info));
        });
        RegisterPrintOperation<PrintableMap>([this](std::ostream&, const void* object) {
            const auto& [id, document] = *reinterpret_cast<const PrintableMap*>(object);
            array_.emplace_back(SvgAsJson(id, document));
        });
    }

private:
    mutable json::Array array_;
};

void ProcessQueries(from::ParseResult parse_result, queries::Handler& handler, Json, std::ostream& output) {
    static const JsonPrintDriver driver(output);
    const Printer printer(driver);
    parse_result.ProcessModifyQueries(handler, printer);
    parse_result.ProcessSetupQueries(handler, printer);
    parse_result.ProcessResponseQueries(handler, printer);
}

} // namespace into

} // namespace transport_catalogue