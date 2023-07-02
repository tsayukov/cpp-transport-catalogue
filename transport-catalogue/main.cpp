#include "request_handler.h"

int main() {
    using namespace transport_catalogue;

    TransportCatalogue database;
    renderer::MapRenderer renderer;
    router::TransportRouter router;

    queries::Handler handler(database, renderer, router);
    handler.ProcessQueries(from::json, std::cin, into::json, std::cout);
}