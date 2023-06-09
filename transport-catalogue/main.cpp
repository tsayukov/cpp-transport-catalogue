#include "unit_tests.h"

#include "reader.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "json_reader.h"

#include <iostream>

int main() {
    using namespace transport_catalogue;

    RunAllTests();

    TransportCatalogue transport_catalogue;
    renderer::MapRenderer renderer;
    query::Handler handler(transport_catalogue, renderer);

    auto queries = reader::GetAllQueries<reader::From::Json>(std::cin);
    query::ProcessBaseQueries(queries, transport_catalogue);
    query::ProcessRenderSettings(queries, renderer);

    handler.RenderMap().Render(std::cout);
}