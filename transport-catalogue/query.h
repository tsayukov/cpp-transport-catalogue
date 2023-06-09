#pragma once

#include "geo.h"
#include "svg.h"
#include "map_renderer.h"

#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

namespace transport_catalogue::query {

enum class Tag {
    _,
    StopCreation,
    StopDistances,
    BusCreation,
    StopInfo,
    BusInfo,
    RenderSettings,
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

using Any = Query<Tag::_>;

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

template<>
struct Query<Tag::RenderSettings> {
    renderer::Settings settings;
};

namespace tests {

inline void TestParseStopCreation();
inline void TestParseBusCreation();
inline void TestParseBusInfo();

} // namespace tests

} // namespace transport_catalogue::query