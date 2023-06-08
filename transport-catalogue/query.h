/// \file
/// Parser that processes a text into queries for filling in and getting info from TransportCatalogue

#pragma once

#include "geo.h"
#include "str_view_handler.h"

#include <string>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <cassert>

namespace transport_catalogue::query {

enum class Tag {
    _,
    StopCreation,
    StopDistances,
    BusCreation,
    StopInfo,
    BusInfo,
};

template<Tag>
struct Query;

template<>
struct Query<Tag::_> {
public:
    template<Tag T>
    explicit Query(Query<T> query)
            : tag_(T)
            , data_(new Query<T>(std::move(query)))
            , deleter_([](Query<Tag::_>& this_ref) noexcept {
                  delete &this_ref.GetData<T>();
              }) {
    }

    Query(const Query& other) = delete;
    Query& operator=(const Query& rhs) = delete;

    Query(Query&& other) noexcept
            : tag_(other.tag_)
            , data_(std::exchange(other.data_, nullptr))
            , deleter_(other.deleter_) {
    }

    Query& operator=(Query&& rhs) noexcept {
        if (this != &rhs) {
            rhs.Swap(*this);
        }
        return *this;
    }

    [[nodiscard]] Tag GetTag() const noexcept {
        return tag_;
    }

    template<Tag T>
    [[nodiscard]] const Query<T>& GetData() const noexcept {
        assert(tag_ == T);
        return *reinterpret_cast<const Query<T>*>(data_);
    }

    template<Tag T>
    [[nodiscard]] Query<T>& GetData() noexcept {
        assert(tag_ == T);
        return *reinterpret_cast<Query<T>*>(data_);
    }

    void Swap(Query& rhs) noexcept {
        using std::swap;
        swap(tag_, rhs.tag_);
        swap(data_, rhs.data_);
        swap(deleter_, rhs.deleter_);
    }

    ~Query() {
        if (data_ != nullptr) {
            deleter_(*this);
        }
    }

private:
    using Deleter = void (*)(Query<Tag::_>&);

    Tag tag_;
    void* data_;
    Deleter deleter_;
};

// Specific queries

template<>
struct Query<Tag::StopDistances> {
    std::string from_stop_name;
    std::unordered_map<std::string, geo::Meter> distances_to_stops;
};

template<>
struct Query<Tag::StopCreation> {
    std::string stop_name;
    geo::Coordinates coordinates;

    Query<Tag::StopDistances> stop_distances_subquery;
};

template<>
struct Query<Tag::BusCreation> {
    enum class RouteView {
        Full,
        Half,
    };

    std::string bus_name;
    RouteView route_view;
    std::vector<std::string> stops;
};

template<>
struct Query<Tag::StopInfo> {
    std::string stop_name;
};

template<>
struct Query<Tag::BusInfo> {
    std::string bus_name;
};

// Parser

using Any = Query<Tag::_>;

template<Tag>
[[nodiscard]] Any Parse(str_view_handler::SplitView split_view);

template<>
[[nodiscard]] inline Any Parse<Tag::StopInfo>(str_view_handler::SplitView split_view) {
    const auto stop_name = split_view.NextStrippedSubstrBefore(':');
    return Any(Query<Tag::StopInfo>{ std::string(stop_name) });
}

template<>
[[nodiscard]] inline Any Parse<Tag::StopCreation>(str_view_handler::SplitView split_view) {
    using namespace std::string_view_literals;
    using namespace str_view_handler;

    const auto stop_name = split_view.NextStrippedSubstrBefore(':');

    Query<Tag::StopCreation> query;
    query.stop_name = std::string(stop_name);
    const geo::Degree latitude = std::stod(std::string(split_view.NextStrippedSubstrBefore(',')));
    const geo::Degree longitude = std::stod(std::string(split_view.NextStrippedSubstrBefore(',')));
    query.coordinates = geo::Coordinates(latitude, longitude);

    query.stop_distances_subquery.from_stop_name = std::string(stop_name);
    while (!split_view.Empty()) {
        const auto distance = std::stod(std::string(split_view.NextStrippedSubstrBefore("m to"sv)));
        const auto to_stop_name = split_view.NextStrippedSubstrBefore(',');
        query.stop_distances_subquery.distances_to_stops.emplace(std::string(to_stop_name), distance);
    }
    return Any(std::move(query));
}

template<>
[[nodiscard]] inline Any Parse<Tag::BusInfo>(str_view_handler::SplitView split_view) {
    const auto bus_name = split_view.NextStrippedSubstrBefore(':');
    return Any(Query<Tag::BusInfo>{ std::string(bus_name) });
}

template<>
[[nodiscard]] inline Any Parse<Tag::BusCreation>(str_view_handler::SplitView split_view) {
    const auto bus_name = split_view.NextStrippedSubstrBefore(':');
    Query<Tag::BusCreation> query;
    query.bus_name = std::string(bus_name);

    const auto [sep, _] = split_view.CanSplit('>', '-');
    using RouteView = Query<Tag::BusCreation>::RouteView;
    query.route_view = (sep == '>') ? RouteView::Full : RouteView::Half;

    while (!split_view.Empty()) {
        const auto stop_name = split_view.NextStrippedSubstrBefore(sep);
        query.stops.emplace_back(stop_name);
    }
    return Any(std::move(query));
}

[[nodiscard]] inline Any Parse(std::string_view text) {
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    using namespace str_view_handler;

    SplitView split_view(text);
    const auto command = split_view.NextStrippedSubstrBefore(' ');
    if (command == "Stop"sv) {
        if (!split_view.CanSplit(':')) {
            return Parse<Tag::StopInfo>(split_view);
        }
        return Parse<Tag::StopCreation>(split_view);
    } else if (command == "Bus"sv) {
        if (!split_view.CanSplit(':')) {
            return Parse<Tag::BusInfo>(split_view);
        }
        return Parse<Tag::BusCreation>(split_view);
    }
    throw std::invalid_argument("No such command '"s + std::string(command) + "'"s);
}

namespace tests {

inline void TestParseStopCreation();
inline void TestParseBusCreation();
inline void TestParseBusInfo();

} // namespace tests

} // namespace transport_catalogue::query