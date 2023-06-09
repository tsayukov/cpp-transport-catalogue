#include "reader.h"

namespace transport_catalogue::reader {

template<>
ResultType<From::Cli> GetAllQueries<From::Cli>(std::istream& input) {
    using namespace std::string_literals;

    unsigned int query_count;
    input >> query_count >> std::ws;

    std::vector<query::Any> queries;
    queries.reserve(query_count);
    for (std::string line; query_count > 0; query_count -= 1, line.clear()) {
        getline(input, line);
        queries.push_back(query::from_cli::Parse(line));
    }
    return queries;
}

template<>
ResultType<From::Json> GetAllQueries<From::Json>(std::istream& input) {
    return query::from_json::Parse(json::Load(input));
}

} // namespace transport_catalogue::reader
