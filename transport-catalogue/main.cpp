#include "transport_catalogue.h"
#include "reader.h"
#include "input_process.h"
#include "output_process.h"

#include<iostream>

int main() {
    using namespace transport_catalogue;

    unsigned int input_query_count;
    std::cin >> input_query_count >> std::ws;

    TransportCatalogue transport_catalogue;

    auto input_queries = reader::ReadQueries(std::cin, input_query_count);
    query::input::BulkProcess(input_queries, &transport_catalogue);

    unsigned int output_query_count;
    std::cin >> output_query_count >> std::ws;

    auto output_queries = reader::ReadQueries(std::cin, output_query_count);
    query::output::BulkProcessOutput(output_queries, std::cout, &transport_catalogue);
}