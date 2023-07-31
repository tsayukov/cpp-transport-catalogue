#include "input_reader.h"

#include "geo.h"
#include "domain.h"
#include "str_view_handler.h"
#include "request_handler.h"

#include <any>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace transport_catalogue::from {

using str_view_handler::SplitView;

using namespace std::string_literals;
using namespace std::string_view_literals;

class CliParser : public Parser {
public:
    void Parse(std::string_view line) {
        parsed_objects_.clear();

        const auto query_type_index = ParseAndGetTypeIndex(SplitView(line));
        const auto& factory = queries::QueryFactory::GetFactory(query_type_index);
        GetResult().PushBack(factory.Construct(*this));
    }

private:
    mutable std::unordered_map<std::string_view, std::any> parsed_objects_;

    std::type_index ParseAndGetTypeIndex(SplitView split_view) {
        const auto command = split_view.NextStrippedSubstrBefore(' ');
        if (command == "Stop"sv) {
            if (!split_view.CanSplit(':')) {
                return ParseStopInfo(split_view);
            }
            return ParseStopCreation(split_view);
        } else if (command == "Bus"sv) {
            if (!split_view.CanSplit(':')) {
                return ParseBusInfo(split_view);
            }
            return ParseBusCreation(split_view);
        } else {
            throw std::invalid_argument("No such command '"s + std::string(command) + "'"s);
        }
    }

    std::any GetObject(std::string_view object_name) const override {
        return std::move(parsed_objects_.at(object_name));
    }

    SplitView ParseName(SplitView split_view) {
        const auto name = split_view.NextStrippedSubstrBefore(':');
        parsed_objects_.emplace("name"sv, std::string(name));
        return split_view;
    }

    std::type_index ParseStopInfo(SplitView split_view) {
        ParseName(split_view);
        parsed_objects_.emplace("id"sv, 0u);
        return typeid(queries::Handler::StopInfo);
    }

    std::type_index ParseBusInfo(SplitView split_view) {
        ParseName(split_view);
        parsed_objects_.emplace("id"sv, 0u);
        return typeid(queries::Handler::BusInfo);
    }

    std::type_index ParseStopCreation(SplitView split_view) {
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
        return typeid(Stop);
    }

    std::type_index ParseBusCreation(SplitView split_view) {
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
        return typeid(Bus);
    }
};

Parser::Result ReadQueries(from::Cli from) {
    unsigned int query_count;
    from.input >> query_count >> std::ws;

    CliParser parser;
    for (std::string line; query_count > 0 && getline(from.input, line); query_count -= 1) {
        parser.Parse(line);
    }
    return parser.ReleaseResult();
}

} // namespace transport_catalogue::from