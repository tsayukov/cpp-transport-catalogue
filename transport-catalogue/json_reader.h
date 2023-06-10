/// \file
/// Process JSON queries to fill TransportCatalogue database and get JSON output

#pragma once

#include "svg.h"
#include "json.h"
#include "reader.h"
#include "input_reader.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "request_handler.h"

#include <optional>

namespace transport_catalogue::query {

void ProcessBaseQueries(reader::ResultType<reader::From::Json>& queries, TransportCatalogue& transport_catalogue);

void ProcessRenderSettings(reader::ResultType<reader::From::Json>& queries, renderer::MapRenderer& renderer);

void ProcessStatQueries(reader::ResultType<reader::From::Json>& queries, std::ostream& output, const Handler& handler);

[[nodiscard]] json::Node AsJsonNode(int id, std::optional<const TransportCatalogue::StopInfo*> stop_info);
[[nodiscard]] json::Node AsJsonNode(int id, std::optional<const TransportCatalogue::BusInfo*> bus_info);
[[nodiscard]] json::Node AsJsonNode(int id, const svg::Document& document);

} // namespace transport_catalogue::query