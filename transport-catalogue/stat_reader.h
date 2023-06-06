/// \file
/// Process output queries and output of responses

#pragma once

#include "query.h"
#include "transport_catalogue.h"

#include <ostream>

namespace transport_catalogue::query::output {

std::ostream& operator<<(std::ostream& output, const TransportCatalogue::BusInfo& bus_info);
std::ostream& operator<<(std::ostream& output, const TransportCatalogue::StopInfo& stop_info);

std::ostream& PrintBusInfo(std::ostream& output, const TransportCatalogue::BusInfo& bus_info);
std::ostream& PrintStopInfo(std::ostream& output, const TransportCatalogue::StopInfo& stop_info);

void ProcessQueries(std::vector<query::Any>&& queries, std::ostream& output,
                    const TransportCatalogue& transport_catalogue);

} // namespace transport_catalogue::query::output