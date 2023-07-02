#pragma once

#include "queries.h"

#include <istream>
#include <memory>
#include <vector>

namespace transport_catalogue::from {

struct Cli {};

inline constexpr Cli cli = Cli{};

[[nodiscard]] ParseResult ReadQueries(from::Cli, std::istream& input);

} // namespace transport_catalogue::from