#pragma once

#include "query.h"
#include "str_view_handler.h"
#include "json.h"

#include <string_view>
#include <vector>

namespace transport_catalogue::query {

namespace from_cli {

[[nodiscard]] query::Any Parse(std::string_view text);

template<Tag>
[[nodiscard]] query::Any Parse(str_view_handler::SplitView split_view) = delete;

template<>
[[nodiscard]] query::Any Parse<Tag::StopInfo>(str_view_handler::SplitView split_view);

template<>
[[nodiscard]] query::Any Parse<Tag::StopCreation>(str_view_handler::SplitView split_view);

template<>
[[nodiscard]] query::Any Parse<Tag::BusInfo>(str_view_handler::SplitView split_view);

template<>
[[nodiscard]] query::Any Parse<Tag::BusCreation>(str_view_handler::SplitView split_view);

} // namespace from_cli

namespace from_json {

template<Tag>
struct ParseVariant {};

class Parser {
public:
    struct Result {
        std::vector<query::Any> base_queries_;
        std::vector<std::pair<int, query::Any>> stat_queries_;
    };

    explicit Parser(const json::Document& document) noexcept;

    Result&& GetResult();

private:
    const json::Document& document_;
    Result result_;

    void ParseBaseQueries(const json::Array& array);

    void ParseStatQueries(const json::Array& array);

    void ParseQuery(const json::Map& map, ParseVariant<Tag::StopInfo>);

    void ParseQuery(const json::Map& map, ParseVariant<Tag::StopCreation>);

    void ParseQuery(const json::Map& map, ParseVariant<Tag::BusInfo>);

    void ParseQuery(const json::Map& map, ParseVariant<Tag::BusCreation>);
};

[[nodiscard]] Parser::Result Parse(const json::Document& document);

} // namespace from_json

} // namespace transport_catalogue::query