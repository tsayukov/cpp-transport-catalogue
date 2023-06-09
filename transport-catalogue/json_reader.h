/// \file
/// \todo add a description

#pragma once

#include "json.h"
#include "reader.h"
#include "input_reader.h"
#include "transport_catalogue.h"
#include "request_handler.h"

#include <optional>

namespace transport_catalogue::query {

void ProcessBaseQueries(reader::ResultType<reader::From::Json>& queries, TransportCatalogue& transport_catalogue);

void ProcessStatQueries(reader::ResultType<reader::From::Json>& queries, std::ostream& output, const Handler& handler);

template<typename ConvertibleType>
[[nodiscard]] json::Node AsJsonNode(int id, ConvertibleType) = delete;

template<>
[[nodiscard]] json::Node AsJsonNode<std::optional<const TransportCatalogue::StopInfo*>>(
        int id, std::optional<const TransportCatalogue::StopInfo*> stop_info);

template<>
[[nodiscard]] json::Node AsJsonNode<std::optional<const TransportCatalogue::BusInfo*>>(
        int id, std::optional<const TransportCatalogue::BusInfo*> bus_info);


} // namespace transport_catalogue::query