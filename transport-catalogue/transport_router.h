#pragma once

#include "geo.h"
#include "domain.h"
#include "number_wrapper.h"
#include "ranges.h"
#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

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
    struct CombineItem {
        constexpr CombineItem() = default;

        Minute time;
    };

    struct WaitItem {
        Minute time;
    };

    struct BusItem {
        Minute time;
        unsigned int span_count = 0;
    };

    using ItemVariant = std::variant<CombineItem, WaitItem, BusItem>;

    class Item : private ItemVariant {
    public:
        using ItemVariant::variant;

        Item operator+(const Item& rhs) const;
        bool operator<(const Item& rhs) const;
        bool operator>(const Item& rhs) const;

        bool operator==(const Item& rhs) const;

        [[nodiscard]] Minute GetTime() const;

        [[nodiscard]] bool IsWaitItem() const noexcept;
        [[nodiscard]] bool IsBusItem() const noexcept;

        [[nodiscard]] const WaitItem& GetWaitItem() const;
        [[nodiscard]] const BusItem& GetBusItem() const;
    };

    void Initialize(Settings settings);

    [[nodiscard]] std::optional<Settings> GetSettings() const noexcept;

    void InitializeRouter(const TransportCatalogue& database);
    void InitializeRouter(const TransportCatalogue& database,
                          graph::DirectedWeightedGraph<Item> graph,
                          graph::Router<Item>::RoutesInternalData routes_internal_data);

    void ReplaceBy(TransportRouter&& other);

    [[nodiscard]] std::optional<std::reference_wrapper<const graph::Router<Item>>> GetRouter() const;

    [[nodiscard]] const graph::DirectedWeightedGraph<Item>& GetGraph() const;

    [[nodiscard]] bool IsInitialized() const noexcept;

    class Result {
    private:
        using Iterator = decltype(std::declval<graph::Router<Item>::RouteInfo>().edges.cbegin());
    public:
        [[nodiscard]] /* implicit */ operator bool() const noexcept;

        [[nodiscard]] Iterator begin() const;
        [[nodiscard]] Iterator end() const;

        [[nodiscard]] Minute GetTotalTime() const;
        [[nodiscard]] graph::VertexId GetVertexId(graph::EdgeId edge_id) const;
        [[nodiscard]] Item GetItem(graph::EdgeId edge_id) const;
        [[nodiscard]] const Stop& GetStopBy(graph::VertexId vertex_id) const;
        [[nodiscard]] const Bus& GetBusBy(graph::EdgeId edge_id) const;
    private:
        friend TransportRouter;
        explicit Result(std::optional<graph::Router<Item>::RouteInfo> route_info, const TransportRouter& router);

        std::optional<graph::Router<Item>::RouteInfo> route_info_;
        const TransportRouter& router_;
    };

    [[nodiscard]] Result GetRouteBetweenStops(StopPtr from_ptr, StopPtr to_ptr) const;

private:
    std::optional<Settings> settings_;
    graph::DirectedWeightedGraph<Item> graph_;
    std::optional<graph::Router<Item>> router_;

    struct Indices {
        std::unordered_map<graph::EdgeId, BusPtr> edge_id_to_bus_;
        std::vector<StopPtr> vertex_id_to_stop_;
        std::unordered_map<StopPtr, graph::VertexId> stop_to_start_waiting_vertex_;
    };

    Indices indices_;

    [[nodiscard]] graph::VertexId GetStartWaitingVertexId(StopPtr stop_ptr) const;
    [[nodiscard]] graph::VertexId GetStartDrivingVertexId(StopPtr stop_ptr) const;

    template<typename StopContainer, typename DistanceGetter>
    void AddEdges(BusPtr bus_ptr, const StopContainer& stops, const DistanceGetter& distance_getter) {
        unsigned int drop_count = 1;
        for (StopPtr stop_ptr : stops) {
            geo::Meter distance_acc;
            StopPtr from_stop_ptr = stop_ptr;
            unsigned int span_count = 0;
            for (StopPtr to_stop_ptr : ranges::Drop(stops, drop_count)) {
                distance_acc += distance_getter(from_stop_ptr->name, to_stop_ptr->name);
                const auto total_time = Minute::ComputeTime(distance_acc, settings_->bus_velocity);
                span_count += 1;
                const auto edge_id = graph_.AddEdge(
                        {GetStartDrivingVertexId(stop_ptr),
                         GetStartWaitingVertexId(to_stop_ptr),
                         BusItem{total_time, span_count}});
                indices_.edge_id_to_bus_.emplace(edge_id, bus_ptr);
                from_stop_ptr = to_stop_ptr;
            }
            drop_count += 1;
        }
    }
};

} // namespace transport_catalogue::router