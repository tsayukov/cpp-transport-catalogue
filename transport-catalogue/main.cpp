#include "request_handler.h"

#include <iostream>
#include <string_view>

using namespace std::string_view_literals;

void PrintUsage(std::ostream& output = std::cerr) {
    output << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    using namespace transport_catalogue;

    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    TransportCatalogue database;
    renderer::MapRenderer renderer;
    router::TransportRouter router;

    queries::Handler handler(database, renderer, router);
    const auto result = handler.ProcessQueries(mode, from::Json{std::cin}, into::Json{std::cout});
    if (!result && result.IsIncorrectMode()) {
        PrintUsage();
        return 1;
    }
}