/// \file
/// Process input queries to fill TransportCatalogue database

#pragma once

#include "query.h"
#include "transport_catalogue.h"

namespace transport_catalogue::query::input {

template<query::Tag Tag>
void Process(Query<Tag>&, TransportCatalogue*) = delete;

template<>
void Process<Tag::StopCreation>(Query<Tag::StopCreation>& query, TransportCatalogue* transport_catalogue_ptr);

template<>
void Process<Tag::StopDistances>(Query<Tag::StopDistances>& query, TransportCatalogue* transport_catalogue_ptr);

template<>
void Process<Tag::BusCreation>(Query<Tag::BusCreation>& query, TransportCatalogue* transport_catalogue_ptr);

void BulkProcess(std::vector<query::Any>& queries, TransportCatalogue* transport_catalogue_ptr);

} // namespace transport_catalogue::query::input