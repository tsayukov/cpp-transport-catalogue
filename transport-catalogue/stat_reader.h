/// \file
/// Process output queries and output of responses

#pragma once

#include "queries.h"

#include <ostream>

namespace transport_catalogue::into {

struct Text {};

inline constexpr Text text = Text{};

void ProcessQueries(from::ParseResult parse_result, queries::Handler& handler, Text, std::ostream& output);

} // namespace transport_catalogue::into