#include "unit_tests.h"

#include "reader.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "json_reader.h"

#include <iostream>

int main() {
    using namespace transport_catalogue;

    RunAllTests();

    TransportCatalogue transport_catalogue;
    query::Handler handler(transport_catalogue);
    auto queries = reader::GetAllQueries<reader::From::Json>(std::cin);
    query::ProcessBaseQueries(queries, transport_catalogue);
    query::ProcessStatQueries(queries, std::cout, handler);
}