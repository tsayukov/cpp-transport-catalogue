#pragma once

#include "queries.h"

#include <istream>

namespace transport_catalogue::from {

struct Cli {};
inline constexpr Cli cli = Cli{};

[[nodiscard]] Parser::Result ReadQueries(from::Cli, std::istream& input);

} // namespace transport_catalogue::from