#include "stat_reader.h"

#include "geo.h"
#include "domain.h"
#include "request_handler.h"

#include <iomanip>
#include <string_view>
#include <optional>

namespace transport_catalogue::into {

using namespace std::string_view_literals;

using StopInfo = std::optional<const queries::Handler::StopInfo*>;
using BusInfo = std::optional<const queries::Handler::BusInfo*>;

std::ostream& operator<<(std::ostream& output, StopInfo stop_info) {
    if (!stop_info.has_value()) {
        return output << "not found"sv;
    }
    const auto& buses = stop_info.value()->buses;
    if (buses.empty()) {
        return output << "no buses"sv;
    }
    output << "buses"sv;
    for (BusPtr bus_ptr : buses) {
        output << " "sv << bus_ptr->name;
    }
    return output;
}

std::ostream& operator<<(std::ostream& output, BusInfo bus_info) {
    if (!bus_info.has_value()) {
        return output << "not found"sv;
    }
    auto bus_stats_ptr = bus_info.value();
    output << bus_stats_ptr->stops_count << " stops on route, "sv
           << bus_stats_ptr->unique_stops_count << " unique stops, "sv
           << std::setprecision(6)
           << bus_stats_ptr->length.route << " route length, "sv
           << bus_stats_ptr->length.route / bus_stats_ptr->length.geo << " curvature"sv;
    return output;
}

std::ostream& PrintStopInfo(std::ostream& output, std::string_view stop_name, StopInfo stop_info) {
    output << "Stop "sv << stop_name << ": "sv;
    return output << stop_info << '\n';
}

std::ostream& PrintBusInfo(std::ostream& output, std::string_view bus_name, BusInfo bus_info) {
    output << "Bus "sv << bus_name << ": "sv;
    return output << bus_info << '\n';
}

const PrintDriver& GetTextPrintDriver(std::ostream& output) {
    using PrintableStopInfo = std::tuple<int, std::string_view, StopInfo>;
    using PrintableBusInfo = std::tuple<int, std::string_view, BusInfo>;

    static PrintDriver driver(output);
    driver.RegisterPrintOperation<PrintableStopInfo>([](std::ostream& output, const void* object) {
        const auto [_, name, stop_info] = *reinterpret_cast<const PrintableStopInfo*>(object);
        PrintStopInfo(output, name, stop_info);
    });
    driver.RegisterPrintOperation<PrintableBusInfo>([](std::ostream& output, const void* object) {
        const auto [_, name, bus_info] = *reinterpret_cast<const PrintableBusInfo*>(object);
        PrintBusInfo(output, name, bus_info);
    });
    return driver;
}

void ProcessQueries(from::Parser::Result parse_result, queries::Handler& handler, into::Text text) {
    const Printer printer(GetTextPrintDriver(text.output));
    parse_result.ProcessModifyQueries(handler, printer);
    parse_result.ProcessResponseQueries(handler, printer);
}

} // namespace transport_catalogue::into