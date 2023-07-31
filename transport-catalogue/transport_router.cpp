#include "transport_router.h"

namespace transport_catalogue::router {

using namespace std::string_literals;

void TransportRouter::Initialize(Settings settings) {
    settings_ = settings;
}

std::optional<Settings> TransportRouter::GetSettings() const noexcept {
    return settings_;
}

void TransportRouter::InitializeRouter(const TransportCatalogue& database) {
    if (!settings_.has_value()) {
        using namespace std::string_literals;
        throw std::logic_error("Settings must be initialized before a router creation"s);
    }

    const auto& stops = database.GetAllStops();
    const auto vertex_count = std::distance(stops.begin(), stops.end()) * 2;
    graph_ = graph::DirectedWeightedGraph<Item>(vertex_count);

    indices_.vertex_id_to_stop_.reserve(vertex_count);
    for (const Stop& stop : stops) {
        indices_.stop_to_start_waiting_vertex_.emplace(&stop, indices_.vertex_id_to_stop_.size() * 2);
        indices_.vertex_id_to_stop_.push_back(&stop);

        graph_.AddEdge({GetStartWaitingVertexId(&stop), GetStartDrivingVertexId(&stop),
                        WaitItem{settings_->bus_wait_time}});
    }

    const auto& buses = database.GetAllBuses();
    const auto distance_getter = [&database](std::string_view from_stop_name, std::string_view to_stop_name) {
        return database.GetDistanceBetweenStops(from_stop_name, to_stop_name).value();
    };
    for (const Bus& bus : buses) {
        AddEdges(&bus, bus.stops, distance_getter);
        if (bus.route_type == Bus::RouteType::Half) {
            AddEdges(&bus, ranges::Reverse(bus.stops), distance_getter);
        }
    }

    router_.emplace(graph_);
}

void TransportRouter::InitializeRouter(const TransportCatalogue& database,
                                       graph::DirectedWeightedGraph<Item> graph,
                                       graph::Router<Item>::RoutesInternalData routes_internal_data) {
    graph_ = std::move(graph);
    router_.emplace(graph_, std::move(routes_internal_data));

    {
        graph::VertexId vertex_id = 0;
        indices_.vertex_id_to_stop_.reserve(graph_.GetVertexCount() / 2);
        for (const Stop& stop : database.GetAllStops()) {
            indices_.stop_to_start_waiting_vertex_.emplace(&stop, vertex_id);
            indices_.vertex_id_to_stop_.push_back(&stop);
            vertex_id += 2;
        }
    }
    {
        graph::EdgeId edge_id = graph_.GetVertexCount() / 2;
        for (const Bus& bus: database.GetAllBuses()) {
            const auto stop_count = bus.stops.size();
            const auto edge_count = ((stop_count - 1) * stop_count) / 2;
            for ([[maybe_unused]] auto _ : ranges::Indices(edge_count)) {
                indices_.edge_id_to_bus_.emplace(edge_id, &bus);
                edge_id += 1;
            }
            if (bus.route_type == Bus::RouteType::Half) {
                for ([[maybe_unused]] auto _ : ranges::Indices(edge_count)) {
                    indices_.edge_id_to_bus_.emplace(edge_id, &bus);
                    edge_id += 1;
                }
            }
        }
    }
}

void TransportRouter::ReplaceBy(TransportRouter&& other) {
    settings_ = other.settings_;
    graph_ = std::move(other.graph_);
    if (other.router_.has_value()) {
        router_.emplace(graph_, other.router_->ReleaseRoutesInternalData());
    }
    indices_ = std::move(other.indices_);
}

std::optional<std::reference_wrapper<const graph::Router<TransportRouter::Item>>> TransportRouter::GetRouter() const {
    return router_;
}

const graph::DirectedWeightedGraph<TransportRouter::Item>& TransportRouter::GetGraph() const {
    return graph_;
}

bool TransportRouter::IsInitialized() const noexcept {
    return settings_.has_value() && router_.has_value();
}

TransportRouter::Result TransportRouter::GetRouteBetweenStops(StopPtr from_ptr, StopPtr to_ptr) const {
    if (!router_.has_value()) {
        throw std::logic_error("Route must be initialized before a route computation"s);
    }

    auto route_info = router_->BuildRoute(
            indices_.stop_to_start_waiting_vertex_.at(from_ptr),
            indices_.stop_to_start_waiting_vertex_.at(to_ptr));

    return Result(std::move(route_info), *this);
}

graph::VertexId TransportRouter::GetStartWaitingVertexId(StopPtr stop_ptr) const {
    return indices_.stop_to_start_waiting_vertex_.at(stop_ptr);
}

graph::VertexId TransportRouter::GetStartDrivingVertexId(StopPtr stop_ptr) const {
    return indices_.stop_to_start_waiting_vertex_.at(stop_ptr) + 1;
}

// Item

TransportRouter::Item TransportRouter::Item::operator+(const Item& rhs) const {
    return CombineItem{GetTime() + rhs.GetTime()};
}

bool TransportRouter::Item::operator<(const TransportRouter::Item& rhs) const {
    return GetTime() < rhs.GetTime();
}

bool TransportRouter::Item::operator>(const TransportRouter::Item& rhs) const {
    return rhs < *this;
}

bool TransportRouter::Item::operator==(const TransportRouter::Item& rhs) const {
    if (static_cast<const ItemVariant&>(*this).index() != static_cast<const ItemVariant&>(rhs).index()) {
        return false;
    }
    if (IsBusItem()) {
        return GetTime() == rhs.GetTime() && GetBusItem().span_count == rhs.GetBusItem().span_count;
    } else {
        return GetTime() == rhs.GetTime();
    }
}

Minute TransportRouter::Item::GetTime() const {
    return std::visit([](const auto& item) {
        return item.time;
    }, static_cast<const ItemVariant&>(*this));
}

bool TransportRouter::Item::IsWaitItem() const noexcept {
    return std::holds_alternative<WaitItem>(*this);
}

bool TransportRouter::Item::IsBusItem() const noexcept {
    return std::holds_alternative<BusItem>(*this);
}

const TransportRouter::WaitItem& TransportRouter::Item::GetWaitItem() const {
    return std::get<WaitItem>(*this);
}

const TransportRouter::BusItem& TransportRouter::Item::GetBusItem() const {
    return std::get<BusItem>(*this);
}

// Result

TransportRouter::Result::operator bool() const noexcept {
    return route_info_.has_value();
}

TransportRouter::Result::Iterator TransportRouter::Result::begin() const {
    return route_info_->edges.begin();
}

TransportRouter::Result::Iterator TransportRouter::Result::end() const {
    return route_info_->edges.end();
}

Minute TransportRouter::Result::GetTotalTime() const {
    return route_info_->weight.GetTime();
}

graph::VertexId TransportRouter::Result::GetVertexId(graph::EdgeId edge_id) const {
    return router_.graph_.GetEdge(edge_id).from;
}

TransportRouter::Item TransportRouter::Result::GetItem(graph::EdgeId edge_id) const {
    return router_.graph_.GetEdge(edge_id).weight;
}

const Stop& TransportRouter::Result::GetStopBy(graph::VertexId vertex_id) const {
    return *router_.indices_.vertex_id_to_stop_[vertex_id / 2];
}

const Bus& TransportRouter::Result::GetBusBy(graph::EdgeId edge_id) const {
    return *router_.indices_.edge_id_to_bus_.at(edge_id);
}

TransportRouter::Result::Result(std::optional<graph::Router<Item>::RouteInfo> route_info, const TransportRouter& router)
        : route_info_(std::move(route_info))
        , router_(router) {
}

} // namespace transport_catalogue::router