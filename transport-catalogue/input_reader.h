/// \file
/// Process input queries to fill TransportCatalogue database

#pragma once

#include "query.h"
#include "transport_catalogue.h"

namespace transport_catalogue::query::input {

void Process(Query<Tag::StopCreation>& query, TransportCatalogue& transport_catalogue);
void Process(Query<Tag::StopDistances>& query, TransportCatalogue& transport_catalogue);
void Process(Query<Tag::BusCreation>& query, TransportCatalogue& transport_catalogue);

void ProcessQueries(std::vector<query::Any>&& queries, TransportCatalogue& transport_catalogue);

} // namespace transport_catalogue::query::input