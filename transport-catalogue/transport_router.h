#pragma once

#include "geo.h"
#include "domain.h"
#include "number_wrapper.h"
#include "ranges.h"
#include "graph.h"
#include "router.h"

#include <optional>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <vector>

namespace transport_catalogue::router {

using namespace number_wrapper;

class KmPerHour
        : public number_wrapper::Base<double, KmPerHour,
                Equal<KmPerHour>, Ordering<KmPerHour>,
                UnaryPlus<KmPerHour>, UnaryMinus<KmPerHour>,
                Add<KmPerHour>, Subtract<KmPerHour>,
                Multiply<KmPerHour, double>, Divide<KmPerHour, double>, Divide<KmPerHour, KmPerHour, double>> {
public:
    using Base::Base;
};

class Minute
        : public number_wrapper::Base<double, Minute,
                Equal<Minute>, Ordering<Minute>,
                UnaryPlus<Minute>, UnaryMinus<Minute>,
                Add<Minute>, Subtract<Minute>,
                Multiply<Minute, double>, Divide<Minute, double>, Divide<Minute, Minute, double>> {
public:
    using Base::Base;

    [[nodiscard]] static Minute ComputeTime(geo::Meter distance, KmPerHour velocity) {
        return Minute{distance.Get() / (velocity * 1000.0 / 60.0).Get()};
    }
};

struct Settings {
    Minute bus_wait_time;
    KmPerHour bus_velocity;
};

class TransportRouter final {
public:
    void Initialize(Settings settings);

    template<typename StopContainer, typename BusContainer, typename DistanceGetter>
    void InitializeRouter(const StopContainer& stops, const BusContainer& buses, const DistanceGetter& distance_getter);

    [[nodiscard]] bool IsInitialized() const noexcept;

    struct CombineItem {
        constexpr CombineItem() = default;

        Minute time;
    };

    struct WaitItem {
        Minute time;
        StopPtr stop_ptr = nullptr;
    };

    struct BusItem {
        Minute time;
        BusPtr bus_ptr = nullptr;
        unsigned int span_count = 0;
    };

    using ItemVariant = std::variant<CombineItem, WaitItem, BusItem>;

    class Item : private ItemVariant {
    public:
        using ItemVariant::variant;

        Item operator+(const Item& rhs) const;
        bool operator<(const Item& rhs) const;
        bool operator>(const Item& rhs) const;

        [[nodiscard]] Minute GetTime() const;

        [[nodiscard]] bool IsWaitItem() const noexcept;
        [[nodiscard]] bool IsBusItem() const noexcept;

        [[nodiscard]] const WaitItem& GetWaitItem() const;
        [[nodiscard]] const BusItem& GetBusItem() const;
    };

    struct Result {
        Minute total_time;
        std::vector<Item> items;
    };

    [[nodiscard]] std::optional<Result> GetRouteBetweenStops(StopPtr from_ptr, StopPtr to_ptr) const;

private:
    std::optional<Settings> settings_;
    graph::DirectedWeightedGraph<Item> graph_;
    std::optional<graph::Router<Item>> router_;

    std::vector<StopPtr> vertex_to_stop_;
    std::unordered_map<StopPtr, graph::VertexId> stop_to_start_waiting_vertex_;

    [[nodiscard]] graph::VertexId GetStartWaitingVertexId(StopPtr stop_ptr) const;

    [[nodiscard]] graph::VertexId GetStartDrivingVertexId(StopPtr stop_ptr) const;

    template<typename StopContainer, typename DistanceGetter>
    void AddingEdges(BusPtr bus_ptr, const StopContainer& stops, const DistanceGetter& distance_getter);

    [[nodiscard]] Result TransformRouteInfoToResult(const graph::Router<Item>::RouteInfo& route_info) const;
};

template<typename StopContainer, typename BusContainer, typename DistanceGetter>
void TransportRouter::InitializeRouter(const StopContainer& stops, const BusContainer& buses,
                                       const DistanceGetter& distance_getter) {
    static_assert(std::is_same_v<typename StopContainer::value_type, Stop>);
    static_assert(std::is_same_v<typename BusContainer::value_type, Bus>);

    if (!settings_.has_value()) {
        using namespace std::string_literals;
        throw std::logic_error("Settings must be initialized before a router creation"s);
    }

    graph_ = graph::DirectedWeightedGraph<Item>(std::distance(stops.begin(), stops.end()) * 2);

    for (const Stop& stop : stops) {
        stop_to_start_waiting_vertex_.emplace(&stop, vertex_to_stop_.size() * 2);
        vertex_to_stop_.push_back(&stop);

        graph_.AddEdge({GetStartWaitingVertexId(&stop), GetStartDrivingVertexId(&stop),
                        WaitItem{settings_->bus_wait_time, &stop}});
    }

    for (const Bus& bus : buses) {
        AddingEdges(&bus, bus.stops, distance_getter);
        if (bus.route_type == Bus::RouteType::Half) {
            AddingEdges(&bus, ranges::Reverse(bus.stops), distance_getter);
        }
    }

    router_.emplace(graph::Router<Item>(graph_));
}

template<typename StopContainer, typename DistanceGetter>
void TransportRouter::AddingEdges(BusPtr bus_ptr, const StopContainer& stops, const DistanceGetter& distance_getter) {
    static_assert(std::is_same_v<typename StopContainer::value_type, StopPtr>);

    unsigned int drop_count = 1;
    for (StopPtr stop_ptr : stops) {
        geo::Meter distance_acc;
        StopPtr from_stop_ptr = stop_ptr;
        unsigned int span_count = 0;
        for (StopPtr to_stop_ptr : ranges::Drop(stops, drop_count)) {
            distance_acc += distance_getter(from_stop_ptr->name, to_stop_ptr->name).value();
            const auto total_time = Minute::ComputeTime(distance_acc, settings_->bus_velocity);
            span_count += 1;
            graph_.AddEdge({GetStartDrivingVertexId(stop_ptr), GetStartWaitingVertexId(to_stop_ptr),
                            BusItem{total_time, bus_ptr, span_count}});
            from_stop_ptr = to_stop_ptr;
        }
        drop_count += 1;
    }
}

} // namespace transport_catalogue::router