#include "transport_router.h"

namespace transport_catalogue::router {

using namespace std::string_literals;

void TransportRouter::Initialize(Settings settings) {
    settings_ = settings;
}

bool TransportRouter::IsInitialized() const noexcept {
    return settings_.has_value() && router_.has_value();
}

std::optional<TransportRouter::Result> TransportRouter::GetRouteBetweenStops(StopPtr from_ptr, StopPtr to_ptr) const {
    if (!router_.has_value()) {
        throw std::logic_error("Route must be initialized before a route computation"s);
    }

    auto route_info = router_->BuildRoute(stop_to_start_waiting_vertex_.at(from_ptr), stop_to_start_waiting_vertex_.at(to_ptr));
    if (!route_info.has_value()) {
        return std::nullopt;
    }

    return TransformRouteInfoToResult(route_info.value());
}

graph::VertexId TransportRouter::GetStartWaitingVertexId(StopPtr stop_ptr) const {
    return stop_to_start_waiting_vertex_.at(stop_ptr);
}

graph::VertexId TransportRouter::GetStartDrivingVertexId(StopPtr stop_ptr) const {
    return stop_to_start_waiting_vertex_.at(stop_ptr) + 1;
}

TransportRouter::Result TransportRouter::TransformRouteInfoToResult(
        const graph::Router<Item>::RouteInfo& route_info) const {
    Result result;
    result.total_time = route_info.weight.GetTime();
    for (const graph::EdgeId edge_id : route_info.edges) {
        const auto& edge = graph_.GetEdge(edge_id);
        result.items.emplace_back(edge.weight);
    }
    return result;
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

Minute TransportRouter::Item::GetTime() const {
    return std::visit([](const auto& item) {
        return item.time;
    }, static_cast<ItemVariant>(*this));
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

} // namespace transport_catalogue::router