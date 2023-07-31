#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <filesystem>
#include <optional>
#include <type_traits>

namespace transport_catalogue::serialization {

struct Settings {
    std::filesystem::path file;
};

class Serializer {
public:
    void Initialize(Settings settings);

    struct SentData {
        const TransportCatalogue& database;
        std::optional<std::reference_wrapper<const renderer::Settings>> render_settings;
        const router::TransportRouter& transport_router;
    };

    void Serialize(SentData sent_data) const;

    struct ReceivedData {
        TransportCatalogue database;
        std::optional<renderer::Settings> render_settings = std::nullopt;
        std::optional<router::TransportRouter> transport_router = std::nullopt;
    };

    [[nodiscard]] ReceivedData Deserialize() const;

private:
    std::optional<Settings> settings_;
};

} // namespace transport_catalogue::serialization