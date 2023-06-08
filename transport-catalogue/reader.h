/// \file
/// Reading queries from input data.
///
/// \e ReadFrom or \e ReadQuery functions template specializations must be implemented
/// to read queries from a specific input.

#pragma once

#include "query.h"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

namespace transport_catalogue::reader {

template<typename Input>
[[nodiscard]] bool ReadFrom(Input& input, std::string& helper_str) = delete;

template<>
[[nodiscard]] inline bool ReadFrom<std::istream>(std::istream& input, std::string& helper_str) {
    return static_cast<bool>(getline(input, helper_str));
}

template<>
[[nodiscard]] inline bool ReadFrom<std::istringstream>(std::istringstream& input, std::string& helper_str) {
    return static_cast<bool>(getline(input, helper_str));
}

template<typename Input>
[[nodiscard]] std::pair<query::Any, bool> ReadQuery(Input& input) {
    std::string helper_str;
    return ReadQuery(input, helper_str);
}

template<typename Input>
[[nodiscard]] std::pair<query::Any, bool> ReadQuery(Input& input, std::string& helper_str) {
    const bool is_read_successful = ReadFrom<Input>(input, helper_str);
    auto result = std::make_pair(query::Parse(helper_str), is_read_successful);
    helper_str.clear();
    return result;
}

template<typename Input>
[[nodiscard]] std::vector<query::Any> ReadQueries(Input& input, unsigned int query_count) {
    std::vector<query::Any> queries;
    queries.reserve(query_count);
    std::string helper_str;
    for (; query_count > 0; query_count -= 1) {
        auto&& [query, is_read_successful] = ReadQuery(input, helper_str);
        if (!is_read_successful) {
            break;
        }
        queries.push_back(std::move(query));
    }
    return queries;
}

[[nodiscard]] inline std::vector<query::Any> GetAllQueries(std::istream& input) {
    unsigned int query_count;
    input >> query_count >> std::ws;
    return ReadQueries(input, query_count);
}

namespace tests {

inline void TestReadQuery();
inline void TestReadQueries();

}

} // namespace transport_catalogue::reader

