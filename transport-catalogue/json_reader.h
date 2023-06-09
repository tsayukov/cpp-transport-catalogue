/// \file
/// \todo add a description

#pragma once

#include "json.h"
#include "reader.h"
#include "input_reader.h"
#include "transport_catalogue.h"
#include "request_handler.h"

namespace transport_catalogue::query {

void ProcessBaseQueries(reader::ResultType<reader::From::Json>& queries, TransportCatalogue& transport_catalogue);

void ProcessStatQueries(reader::ResultType<reader::From::Json>& queries, std::ostream& output, const Handler& handler);

template<typename ConvertibleType>
[[nodiscard]] json::Node AsJsonNode(int id, ConvertibleType&&) = delete;

template<>
[[nodiscard]] json::Node AsJsonNode<TransportCatalogue::StopInfo>(int id, TransportCatalogue::StopInfo&& stop_info);

template<>
[[nodiscard]] json::Node AsJsonNode<TransportCatalogue::BusInfo>(int id, TransportCatalogue::BusInfo&& bus_info);


} // namespace transport_catalogue::query