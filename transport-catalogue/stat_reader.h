/// \file
/// Process output queries and output of responses

#pragma once

#include "queries.h"

#include <ostream>

namespace transport_catalogue::into {

struct Text {
    std::ostream& output;
};

void ProcessQueries(from::Parser::Result parse_result, queries::Handler& handler, into::Text text);

} // namespace transport_catalogue::into