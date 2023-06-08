#include "domain.h"

namespace transport_catalogue {

geo::Meter GetGeoDistance(const Stop& from, const Stop& to) noexcept {
    return geo::ComputeDistance(from.coordinates, to.coordinates);
}

} // namespace transport_catalogue