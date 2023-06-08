/// \file
/// Facade for processing queries to TransportCatalogue database

#pragma once

#include "svg.h"
#include "transport_catalogue.h"

#include <string_view>

namespace transport_catalogue::query {

class Handler {
public:
    explicit Handler(const TransportCatalogue& database/*, const renderer::MapRenderer& renderer_*/) noexcept;

    [[nodiscard]] TransportCatalogue::BusInfo GetBusInfo(std::string_view bus_name) const;

    [[nodiscard]] TransportCatalogue::StopInfo GetStopInfo(std::string_view stop_name) const;

//    svg::Document RenderMap() const;

private:
    const TransportCatalogue& database_;
//    const renderer::MapRenderer& renderer_;
};

} // namespace transport_catalogue::query