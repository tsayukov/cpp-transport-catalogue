#include "request_handler.h"

namespace transport_catalogue::query {

Handler::Handler(const TransportCatalogue& database/*, const renderer::MapRenderer& renderer_*/) noexcept
        : database_(database) {
}

TransportCatalogue::BusInfo Handler::GetBusInfo(std::string_view bus_name) const {
    return database_.GetBusInfo(bus_name);
}

TransportCatalogue::StopInfo Handler::GetStopInfo(std::string_view stop_name) const {
    return database_.GetStopInfo(stop_name);
}

} // namespace transport_catalogue::query