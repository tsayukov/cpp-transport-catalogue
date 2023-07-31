/// \file
/// Process JSON queries to fill TransportCatalogue database and get JSON output

#pragma once

#include "queries.h"
#include "svg.h"
#include "json.h"

namespace transport_catalogue {

namespace from {

struct Json {
    std::istream& input;
};

[[nodiscard]] Parser::Result ReadQueries(from::Json from);

} // namespace from

namespace into {

struct Json {
    std::ostream& output;
};

void ProcessQueries(from::Parser::Result parse_result, queries::Handler& handler, into::Json into);

} // namespace into

} // namespace transport_catalogue