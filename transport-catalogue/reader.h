/// \file
/// Reading queries from input data.
///
/// \e ReadFrom or \e ReadQuery functions template specializations must be implemented
/// to read queries from a specific input.

#pragma once

#include "query.h"

#include <string>
#include <sstream>
#include <iostream>
#include <vector>

namespace transport_catalogue::reader {

template<typename Input>
[[nodiscard]] bool ReadFrom(Input& input, std::string* helper_str_ptr) = delete;

template<>
[[nodiscard]] inline bool ReadFrom<std::istream>(std::istream& input, std::string* helper_str_ptr) {
    assert(helper_str_ptr != nullptr);
    return static_cast<bool>(getline(input, *helper_str_ptr));
}

template<>
[[nodiscard]] inline bool ReadFrom<std::istringstream>(std::istringstream& input, std::string* helper_str_ptr) {
    assert(helper_str_ptr != nullptr);
    return static_cast<bool>(getline(input, *helper_str_ptr));
}

template<typename Input>
std::pair<query::Any, bool> ReadQuery(Input& input, std::string* helper_str_ptr = nullptr) {
    std::string text;
    if (helper_str_ptr == nullptr) {
        helper_str_ptr = &text;
    }
    const bool is_read_successful = ReadFrom<Input>(input, helper_str_ptr);
    auto result = std::make_pair(query::Parse(*helper_str_ptr), is_read_successful);
    helper_str_ptr->clear();
    return result;
}

template<typename Input>
std::vector<query::Any> ReadQueries(Input& input, unsigned int query_count = std::numeric_limits<unsigned int>::max()) {
    std::vector<query::Any> queries;
    std::string helper_str;
    for (; query_count > 0; query_count -= 1) {
        auto&& [query, is_read_successful] = ReadQuery(input, &helper_str);
        if (!is_read_successful) {
            break;
        }
        queries.push_back(std::move(query));
    }
    return queries;
}

} // namespace transport_catalogue::reader

