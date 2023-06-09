#pragma once

#include "unit_test_tools.h"

#include "str_view_handler.h"
#include "query.h"
#include "reader.h"

#include <sstream>

namespace str_view_handler::tests {

using namespace std::string_view_literals;

inline void TestLeftStrip() {
    ASSERT(LeftStrip(""sv).empty());

    ASSERT_EQUAL(LeftStrip("a"sv), "a"sv);
    ASSERT_EQUAL(LeftStrip("a cat"sv), "a cat"sv);

    ASSERT_EQUAL(LeftStrip(" a"sv), "a"sv);
    ASSERT_EQUAL(LeftStrip("   a"sv), "a"sv);
    ASSERT_EQUAL(LeftStrip("   a cat"sv), "a cat"sv);

    ASSERT_EQUAL(LeftStrip("a cat  "sv), "a cat  "sv);
}

inline void TestRightStrip() {
    ASSERT(RightStrip(""sv).empty());

    ASSERT_EQUAL(RightStrip("a"sv), "a"sv);
    ASSERT_EQUAL(RightStrip("a cat"sv), "a cat"sv);

    ASSERT_EQUAL(RightStrip("a "sv), "a"sv);
    ASSERT_EQUAL(RightStrip("a   "sv), "a"sv);
    ASSERT_EQUAL(RightStrip("a cat   "sv), "a cat"sv);

    ASSERT_EQUAL(RightStrip("  a cat"sv), "  a cat"sv);
}

inline void TestStrip() {
    ASSERT(Strip(""sv).empty());

    ASSERT_EQUAL(Strip("a"sv), "a"sv);
    ASSERT_EQUAL(Strip("a cat"sv), "a cat"sv);

    ASSERT_EQUAL(Strip(" a"sv), "a"sv);
    ASSERT_EQUAL(Strip("   a"sv), "a"sv);
    ASSERT_EQUAL(Strip("  a cat"sv), "a cat"sv);

    ASSERT_EQUAL(Strip("a "sv), "a"sv);
    ASSERT_EQUAL(Strip("a   "sv), "a"sv);
    ASSERT_EQUAL(Strip("a cat   "sv), "a cat"sv);

    ASSERT_EQUAL(Strip("   a   "sv), "a"sv);
    ASSERT_EQUAL(Strip("  a cat  "sv), "a cat"sv);
}

inline void TestSplitView() {
    auto str_view = "Bus 142 express: Stop 1 - Stop 2 - Closed stop - Stop 3"sv;
    SplitView split_view(str_view);

    ASSERT_EQUAL(split_view.Rest(), str_view);
    ASSERT_EQUAL(split_view.NextSubstrBefore(' '), "Bus"sv);
    ASSERT_EQUAL(split_view.CanSplit(':'), true);
    ASSERT_EQUAL(split_view.NextSubstrBefore(':'), "142 express"sv);
    ASSERT_EQUAL(split_view.CanSplit('>'), false);
    ASSERT_EQUAL((split_view.CanSplit('-', '>')), (std::make_pair('-', true)));
    ASSERT_EQUAL(split_view.NextStrippedSubstrBefore('-'), "Stop 1"sv);
    ASSERT_EQUAL((split_view.CanSplit('?', ':' , '=')), (std::make_pair('\0', false)));
    ASSERT_EQUAL(split_view.NextStrippedSubstrBefore('-'), "Stop 2"sv);
    split_view.SkipSubstrBefore('-');
    ASSERT_EQUAL(split_view.NextStrippedSubstrBefore('-'), "Stop 3"sv);
    ASSERT_EQUAL(split_view.NextStrippedSubstrBefore('-'), ""sv);
    ASSERT_EQUAL(split_view.Rest(), ""sv);
}

inline void RunAllTests() {
    RUN_TEST(TestLeftStrip);
    RUN_TEST(TestRightStrip);
    RUN_TEST(TestStrip);
    RUN_TEST(TestSplitView);
}

} // namespace str_view_handler::tests

namespace transport_catalogue {

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace query::tests {

inline void TestParseStopCreation() {
    {
        const std::string_view text = "Stop A: 0, 0"sv;
        const auto query = from_cli::Parse(text).GetData<Tag::StopCreation>();
        const auto answer = Query<Tag::StopCreation>{ "A"s, geo::Coordinates(), {} };
        ASSERT((tie(query.stop_name, query.coordinates)) == (tie(answer.stop_name, answer.coordinates)));
    }
    {
        const std::string_view text = "Stop Stop 4A: 55.60, 0.25"sv;
        const auto query = from_cli::Parse(text).GetData<Tag::StopCreation>();
        const auto answer = Query<Tag::StopCreation>{ "Stop 4A"s, geo::Coordinates(55.60, 0.25), {} };
        ASSERT((tie(query.stop_name, query.coordinates)) == (tie(answer.stop_name, answer.coordinates)));
    }
    {
        const std::string_view text = "Stop Stop 4A: +55.60, -0.25"sv;
        const auto query = from_cli::Parse(text).GetData<Tag::StopCreation>();
        const auto answer = Query<Tag::StopCreation>{ "Stop 4A"s, geo::Coordinates(55.60, -0.25), {} };
        ASSERT((tie(query.stop_name, query.coordinates)) == (tie(answer.stop_name, answer.coordinates)));
    }
}

inline void TestParseBusCreation() {
    using RouteView = Query<Tag::BusCreation>::RouteView;
    {
        const std::string_view text = "Bus 145: Stop > Stop 1"sv;
        const auto query = from_cli::Parse(text).GetData<Tag::BusCreation>();
        const auto answer = Query<Tag::BusCreation>{ "145"s, RouteView::Full, {"Stop"s, "Stop 1"s} };
        ASSERT((tie(query.bus_name, query.route_view, query.stops))
               == (tie(answer.bus_name, answer.route_view, answer.stops)));
    }
    {
        const std::string_view text = "Bus 145 express: Stop > Stop 1 > Stop 2 F > Stop 3"sv;
        const auto query = from_cli::Parse(text).GetData<Tag::BusCreation>();
        const auto answer = Query<Tag::BusCreation>{
            "145 express"s, RouteView::Full,
            {"Stop"s, "Stop 1"s, "Stop 2 F"s, "Stop 3"s} };
        ASSERT((tie(query.bus_name, query.route_view, query.stops))
               == (tie(answer.bus_name, answer.route_view, answer.stops)));
    }
    {
        const std::string_view text = "Bus 145 express: Stop - Stop 1 - Stop 2 F - Stop 3"sv;
        const auto query = from_cli::Parse(text).GetData<Tag::BusCreation>();
        const auto answer = Query<Tag::BusCreation>{
                "145 express"s, RouteView::Half,
                {"Stop"s, "Stop 1"s, "Stop 2 F"s, "Stop 3"s} };
        ASSERT((tie(query.bus_name, query.route_view, query.stops))
               == (tie(answer.bus_name, answer.route_view, answer.stops)));
    }
}

inline void TestParseBusInfo() {
    {
        const std::string_view text = "Bus 145"sv;
        const auto query = from_cli::Parse(text).GetData<Tag::BusInfo>();
        const auto answer = Query<Tag::BusInfo>{ "145"s };
        ASSERT(query.bus_name == answer.bus_name);
    }
    {
        const std::string_view text = "Bus 145 express"sv;
        const auto query = from_cli::Parse(text).GetData<Tag::BusInfo>();
        const auto answer = Query<Tag::BusInfo>{ "145 express"s };
        ASSERT(query.bus_name == answer.bus_name);
    }
}

inline void RunAllTests() {
    RUN_TEST(TestParseStopCreation);
    RUN_TEST(TestParseBusCreation);
    RUN_TEST(TestParseBusInfo);
}

} // namespace query::tests

namespace reader::tests {

using namespace query;

inline void TestReadQuery() {
    std::istringstream input("1\nStop A: 0, 0"s, std::ios_base::in);
    const auto any_query = std::move(GetAllQueries<From::Cli>(input).front());
    const auto& query = any_query.GetData<Tag::StopCreation>();
    const auto answer = Query<Tag::StopCreation>{ "A"s, geo::Coordinates(), {} };
    ASSERT((tie(query.stop_name, query.coordinates)) == (tie(answer.stop_name, answer.coordinates)));
}

inline void TestReadQueries() {
    using RouteView = Query<Tag::BusCreation>::RouteView;

    std::istringstream input(
            "3\n"
            "Stop A: 0, 0\n"
            "Bus 145 express: Stop > Stop 1 > Stop 2 F > Stop 3\n"
            "Bus 145\n"s,
            std::ios_base::in);
    const auto queries = GetAllQueries<From::Cli>(input);

    const auto& query_1 = queries[0].GetData<Tag::StopCreation>();
    const auto answer_1 = Query<Tag::StopCreation>{ "A"s, geo::Coordinates(), {} };
    ASSERT((tie(query_1.stop_name, query_1.coordinates)) == (tie(answer_1.stop_name, answer_1.coordinates)));

    const auto& query_2 = queries[1].GetData<Tag::BusCreation>();
    const auto answer_2 = Query<Tag::BusCreation>{
            "145 express"s, RouteView::Full,
            {"Stop"s, "Stop 1"s, "Stop 2 F"s, "Stop 3"s} };
    ASSERT((tie(query_2.bus_name, query_2.route_view, query_2.stops))
           == (tie(answer_2.bus_name, answer_2.route_view, answer_2.stops)));

    const auto& query_3 = queries[2].GetData<Tag::BusInfo>();
    const auto answer_3 = Query<Tag::BusInfo>{ "145"s };
    ASSERT(query_3.bus_name == answer_3.bus_name);
}

inline void RunAllTests() {
    RUN_TEST(TestReadQuery);
    RUN_TEST(TestReadQueries);
}

} // namespace reader::tests

inline void RunAllTests() {
    str_view_handler::tests::RunAllTests();
    query::tests::RunAllTests();
    reader::tests::RunAllTests();
}

} // namespace transport_catalogue