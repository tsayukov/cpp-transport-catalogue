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
    using RouteView = Query<Tag::BusCreation>::RouteView;
    query.route_view = (sep == '>') ? RouteView::Full : RouteView::Half;

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
    ParseBaseQueries(root_map.at("base_requests"s).AsArray());
    ParseStatQueries(root_map.at("stat_requests"s).AsArray());
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

    using RouteView = Query<Tag::BusCreation>::RouteView;
    query.route_view = map.at("is_roundtrip"s).AsBool() ? RouteView::Full : RouteView::Half;

    for (const auto& node : map.at("stops"s).AsArray()) {
        const auto& stop_name = node.AsString();
        query.stops.emplace_back(stop_name);
    }

    result_.base_queries_.emplace_back(std::move(query));
}

} // namespace from_json

} // namespace transport_catalogue::query