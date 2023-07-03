/// \file
/// Process JSON queries to fill TransportCatalogue database and get JSON output

#pragma once

#include "queries.h"

namespace transport_catalogue {

namespace from {

class Json {};

inline constexpr Json json = Json{};

[[nodiscard]] ParseResult ReadQueries(from::Json, std::istream& input);

} // namespace from

namespace into {

class Json {};

inline constexpr Json json = Json{};

void ProcessQueries(from::ParseResult parse_result, queries::Handler& handler, Json, std::ostream& output);

} // namespace into

} // namespace transport_catalogue