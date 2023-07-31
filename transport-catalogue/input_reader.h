#pragma once

#include "queries.h"

#include <istream>

namespace transport_catalogue::from {

struct Cli {
    std::istream& input;
};

[[nodiscard]] Parser::Result ReadQueries(from::Cli from);

} // namespace transport_catalogue::from