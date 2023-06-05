/// \file
/// Process output queries and output of responses

#pragma once

#include "query.h"
#include "transport_catalogue.h"

#include <ostream>
#include <iomanip>

namespace transport_catalogue::query::output {

template<query::Tag Tag, typename ResultType>
ResultType ProcessOutput(Query<Tag>&, const TransportCatalogue*) = delete;

template<>
TransportCatalogue::BusInfo ProcessOutput<Tag::BusInfo, TransportCatalogue::BusInfo>(
        Query<Tag::BusInfo>& query, const TransportCatalogue* transport_catalogue_ptr);

template<>
TransportCatalogue::StopInfo ProcessOutput<Tag::StopInfo, TransportCatalogue::StopInfo>(
        Query<Tag::StopInfo>& query, const TransportCatalogue* transport_catalogue_ptr);

std::ostream& operator<<(std::ostream& output, const TransportCatalogue::BusInfo& bus_info);

std::ostream& operator<<(std::ostream& output, const TransportCatalogue::StopInfo& stop_info);

void BulkProcessOutput(std::vector<query::Any>& queries, std::ostream& output,
                       const TransportCatalogue* transport_catalogue_ptr);

} // namespace transport_catalogue::query::output