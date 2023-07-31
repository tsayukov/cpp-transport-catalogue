#include "json_reader.h"

#include "geo.h"
#include "domain.h"
#include "json_builder.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "request_handler.h"

#include <algorithm>
#include <sstream>
#include <string_view>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace transport_catalogue {

namespace from {

using namespace std::string_literals;
using namespace std::string_view_literals;

class JsonParser : public Parser {
public:
    JsonParser()
            : object_getters_({{"name"sv,                   &JsonParser::GetName},
                               {"id"sv,                     &JsonParser::GetId},
                               {"coordinates"sv,            &JsonParser::GetCoordinates},
                               {"distances"sv,              &JsonParser::GetDistances},
                               {"stop_names"sv,             &JsonParser::GetStopNames},
                               {"route_type"sv,             &JsonParser::GetRouteType},
                               {"renderer_settings"sv,      &JsonParser::GetRendererSettings},
                               {"router_settings"sv,        &JsonParser::GetRouterSettings},
                               {"serialization_settings"sv, &JsonParser::GetSerializationSettings},
                               {"from"sv,                   &JsonParser::GetStartStopName},
                               {"to"sv,                     &JsonParser::GetEndStopName}}) {
    }

    void Parse(const json::Document& document) {
        current_node_ = nullptr;
        for (const auto& [request_type, nodes] : document.GetRoot().AsDict()) {
            if (nodes.IsArray()) {
                for (const auto& node : nodes.AsArray()) {
                    current_node_ = &node;
                    const auto query_type_index = GetTypeIndex(request_type);
                    const auto& factory = queries::QueryFactory::GetFactory(query_type_index);
                    GetResult().PushBack(factory.Construct(*this));
                }
            } else if (nodes.IsDict()) {
                current_node_ = &nodes;
                const auto query_type_index = GetTypeIndex(request_type);
                const auto& factory = queries::QueryFactory::GetFactory(query_type_index);
                GetResult().PushBack(factory.Construct(*this));
            }
        }
    }

private:
    const json::Node* current_node_ = nullptr;

    using ObjectGetter = std::any(JsonParser::*)() const;

    std::unordered_map<std::string_view, ObjectGetter> object_getters_;

    [[nodiscard]] std::type_index GetTypeIndex(std::string_view request_type) const {
        const auto& dict = current_node_->AsDict();
        if (request_type == "base_requests"sv) {
            const auto& type = dict.at("type");
            if (type == "Stop"s) {
                return typeid(Stop);
            } else if (type == "Bus"s) {
                return typeid(Bus);
            }
        } else if (request_type == "stat_requests"s) {
            const auto& type = dict.at("type");
            if (type == "Stop"s) {
                return typeid(queries::Handler::StopInfo);
            } else if (type == "Bus"s) {
                return typeid(queries::Handler::BusInfo);
            } else if (type == "Map"s) {
                return typeid(renderer::MapRenderer);
            } else if (type == "Route"s) {
                return typeid(router::TransportRouter);
            }
        } else if (request_type == "render_settings"s) {
            return typeid(renderer::Settings);
        } else if (request_type == "routing_settings"s) {
            return typeid(router::Settings);
        } else if (request_type == "serialization_settings"s) {
            return typeid(serialization::Settings);
        }
        throw std::invalid_argument("No such request type '"s + std::string(request_type) + "'"s);
    }

    [[nodiscard]] std::any GetObject(std::string_view object_name) const override {
        const auto f = object_getters_.at(object_name);
        return (this->*f)();
    }

    [[nodiscard]] std::any GetName() const {
        std::string name = current_node_->AsDict().at("name"s).AsString();
        return std::make_any<std::string>(std::move(name));
    }

    [[nodiscard]] std::any GetId() const {
        return current_node_->AsDict().at("id"s).AsInt();
    }

    [[nodiscard]] std::any GetCoordinates() const {
        const auto& dict = current_node_->AsDict();
        const auto latitude = geo::Degree{dict.at("latitude"s).AsDouble()};
        const auto longitude = geo::Degree{dict.at("longitude"s).AsDouble()};
        return geo::Coordinates(latitude, longitude);
    }

    [[nodiscard]] std::any GetDistances() const {
        const auto& dict = current_node_->AsDict();
        const auto& map = dict.at("road_distances"s).AsDict();
        std::unordered_map<std::string, geo::Meter> distances;
        distances.reserve(map.size());
        for (const auto& [to_stop_name, node] : map) {
            const auto distance = geo::Meter{node.AsDouble()};
            distances.emplace(to_stop_name, distance);
        }
        return std::make_any<decltype(distances)>(std::move(distances));
    }

    [[nodiscard]] std::any GetStopNames() const {
        const auto& dict = current_node_->AsDict();
        const auto& array = dict.at("stops"s).AsArray();
        std::vector<std::string> stop_names;
        stop_names.reserve(array.size());
        for (const auto& node : array) {
            const auto& stop_name = node.AsString();
            stop_names.emplace_back(stop_name);
        }
        return std::make_any<decltype(stop_names)>(std::move(stop_names));
    }

    [[nodiscard]] std::any GetRouteType() const {
        return (current_node_->AsDict().at("is_roundtrip"s).AsBool()) ? Bus::RouteType::Full : Bus::RouteType::Half;
    }

    [[nodiscard]] std::any GetRendererSettings() const {
        const auto& dict = current_node_->AsDict();
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

        return std::make_any<renderer::Settings>(std::move(rs));
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

    [[nodiscard]] std::any GetRouterSettings() const {
        const auto& dict = current_node_->AsDict();
        router::Settings rs;
        rs.bus_wait_time = router::Minute{dict.at("bus_wait_time"s).AsDouble()};
        rs.bus_velocity = router::KmPerHour{dict.at("bus_velocity"s).AsDouble()};
        return rs;
    }

    [[nodiscard]] std::any GetSerializationSettings() const {
        const auto& dict = current_node_->AsDict();
        serialization::Settings ss;
        ss.file = dict.at("file"s).AsString();
        return std::make_any<decltype(ss)>(std::move(ss));
    }

    [[nodiscard]] std::any GetStartStopName() const {
        std::string stop_name = current_node_->AsDict().at("from"s).AsString();
        return std::make_any<std::string>(std::move(stop_name));
    }

    [[nodiscard]] std::any GetEndStopName() const {
        std::string stop_name = current_node_->AsDict().at("to"s).AsString();
        return std::make_any<std::string>(std::move(stop_name));
    }
};

[[nodiscard]] Parser::Result ReadQueries(from::Json from) {
    JsonParser parser;
    parser.Parse(json::Load(from.input));
    return parser.ReleaseResult();
}

} // namespace from

namespace into {

using namespace std::string_literals;

using StopInfo = std::optional<const queries::Handler::StopInfo*>;
using BusInfo = std::optional<const queries::Handler::BusInfo*>;

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

json::Node RouteAsJson(int id, const queries::Handler::RouteResult& route_result) {
    auto builder = json::Builder{};
    auto dict_builder = builder
            .StartDict()
                .Key("request_id"s).Value(id);

    if (!route_result) {
        return dict_builder
                    .Key("error_message"s).Value("not found"s)
                .EndDict()
                .Build();
    }

    json::Array items;
    items.reserve(std::distance(route_result.begin(), route_result.end()));
    for (const auto edge_id : route_result) {
        json::Node item_node;
        const auto& item = route_result.GetItem(edge_id);
        if (item.IsWaitItem()) {
            const auto& wait_item = item.GetWaitItem();
            const std::string& stop_name = route_result.GetStopBy(route_result.GetVertexId(edge_id)).name;
            item_node = json::Builder{}
                    .StartDict()
                        .Key("type"s).Value("Wait"s)
                        .Key("stop_name"s).Value(stop_name)
                        .Key("time"s).Value(wait_item.time.Get())
                    .EndDict()
                    .Build();
        } else if (item.IsBusItem()) {
            const auto& bus_item = item.GetBusItem();
            const std::string& bus_name = route_result.GetBusBy(edge_id).name;
            item_node = json::Builder{}
                    .StartDict()
                        .Key("type"s).Value("Bus"s)
                        .Key("bus"s).Value(bus_name)
                        .Key("span_count"s).Value(static_cast<int>(bus_item.span_count))
                        .Key("time"s).Value(bus_item.time.Get())
                    .EndDict()
                    .Build();
        }
        items.emplace_back(std::move(item_node));
    }

    return dict_builder
                .Key("total_time"s).Value(route_result.GetTotalTime().Get())
                .Key("items"s).Value(std::move(items))
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
        json::Print(json::Document(json::Node(std::move(array_))), GetOutput());
    }

    void Initialize() {
        using PrintableStopInfo = std::tuple<int, std::string_view, StopInfo>;
        using PrintableBusInfo = std::tuple<int, std::string_view, BusInfo>;
        using PrintableMap = std::tuple<int, svg::Document>;
        using PrintableRoute = std::tuple<int, queries::Handler::RouteResult>;

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
        RegisterPrintOperation<PrintableRoute>([this](std::ostream&, const void* object) {
            const auto& [id, route_result] = *reinterpret_cast<const PrintableRoute*>(object);
            array_.emplace_back(RouteAsJson(id, route_result));
        });
    }

private:
    mutable json::Array array_;
};

void ProcessQueries(from::Parser::Result parse_result, queries::Handler& handler, into::Json into) {
    static const JsonPrintDriver driver(into.output);
    const Printer printer(driver);
    parse_result.ProcessModifyQueries(handler, printer);
    parse_result.ProcessResponseQueries(handler, printer);
}

} // namespace into

} // namespace transport_catalogue