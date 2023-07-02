#include "input_reader.h"

#include "geo.h"
#include "domain.h"
#include "str_view_handler.h"

#include <any>
#include <unordered_map>

namespace transport_catalogue::from {

using str_view_handler::SplitView;

using namespace std::string_literals;
using namespace std::string_view_literals;

class CliParser : public Parser {
public:
    void Parse(std::string_view line) {
        query_type_ = {};
        parsed_objects_.clear();

        SplitView split_view(line);
        const auto command = split_view.NextStrippedSubstrBefore(' ');
        if (command == "Stop"sv) {
            if (!split_view.CanSplit(':')) {
                return ParseStopInfo(split_view);
            }
            ParseStopCreation(split_view);
        } else if (command == "Bus"sv) {
            if (!split_view.CanSplit(':')) {
                return ParseBusInfo(split_view);
            }
            ParseBusCreation(split_view);
        } else {
            throw std::invalid_argument("No such command '"s + std::string(command) + "'"s);
        }
    }

    QueryType GetQueryType() const override {
        return query_type_;
    }

private:
    QueryType query_type_;
    mutable std::unordered_map<std::string_view, std::any> parsed_objects_;

    std::any GetObject(std::string_view object_name) const override {
        return std::move(parsed_objects_.at(object_name));
    }

    SplitView ParseName(SplitView split_view) {
        const auto name = split_view.NextStrippedSubstrBefore(':');
        parsed_objects_.emplace("name"sv, std::string(name));
        return split_view;
    }

    void ParseStopInfo(SplitView split_view) {
        ParseName(split_view);
        parsed_objects_.emplace("id"sv, 0u);
        query_type_ = "stop_info"sv;
    }

    void ParseBusInfo(SplitView split_view) {
        ParseName(split_view);
        parsed_objects_.emplace("id"sv, 0u);
        query_type_ = "bus_info"sv;
    }

    void ParseStopCreation(SplitView split_view) {
        split_view = ParseName(split_view);

        const auto latitude = geo::Degree{std::stod(std::string(split_view.NextStrippedSubstrBefore(',')))};
        const auto longitude = geo::Degree{std::stod(std::string(split_view.NextStrippedSubstrBefore(',')))};
        parsed_objects_.emplace("coordinates"sv, geo::Coordinates(latitude, longitude));

        std::unordered_map<std::string, geo::Meter> distances;
        while (!split_view.Empty()) {
            const auto distance = geo::Meter{std::stod(std::string(split_view.NextStrippedSubstrBefore("m to"sv)))};
            const auto to_stop_name = split_view.NextStrippedSubstrBefore(',');
            distances.emplace(std::string(to_stop_name), distance);
        }
        parsed_objects_.emplace("distances"sv, std::move(distances));

        query_type_ = "stop_creation"sv;
    }

    void ParseBusCreation(SplitView split_view) {
        split_view = ParseName(split_view);

        const auto [sep, _] = split_view.CanSplit('>', '-');
        const auto route_type = (sep == '>') ? Bus::RouteType::Full : Bus::RouteType::Half;
        parsed_objects_.emplace("route_type"sv, route_type);

        std::vector<std::string> stop_names;
        while (!split_view.Empty()) {
            const auto stop_name = split_view.NextStrippedSubstrBefore(sep);
            stop_names.emplace_back(stop_name);
        }
        parsed_objects_.emplace("stop_names"sv, std::move(stop_names));

        query_type_ = "bus_creation"sv;
    }
};

ParseResult ReadQueries(from::Cli, std::istream& input) {
    unsigned int query_count;
    input >> query_count >> std::ws;

    ParseResult result;
    CliParser parser;
    for (std::string line; query_count > 0 && getline(input, line); query_count -= 1) {
        parser.Parse(line);
        const auto query_type = parser.GetQueryType();
        const auto& factory = queries::QueryFactory::GetFactory(query_type);
        result.PushBack(factory.Construct(parser), parser.GetQueryType());
    }
    return result;
}

} // namespace transport_catalogue::from