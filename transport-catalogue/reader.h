/// \file
/// Reading queries from input data.

#pragma once

#include "query.h"
#include "parser.h"
#include "json.h"

#include <iostream>
#include <string>
#include <vector>

namespace transport_catalogue::reader {

enum class From {
    Cli,
    Json,
};

template<From>
struct Result;

template<>
struct Result<From::Cli> {
    using type = std::vector<query::Any>;
};

template<>
struct Result<From::Json> {
    using type = query::from_json::Parser::Result;
};

template<From FromTag>
using ResultType = typename Result<FromTag>::type;

template<From FromTag, typename Result = ResultType<FromTag>>
[[nodiscard]] Result GetAllQueries(std::istream& input) = delete;

template<>
[[nodiscard]] ResultType<From::Cli> GetAllQueries<From::Cli>(std::istream& input);

template<>
[[nodiscard]] ResultType<From::Json> GetAllQueries<From::Json>(std::istream& input);

namespace tests {

inline void TestReadQuery();
inline void TestReadQueries();

}

} // namespace transport_catalogue::reader

