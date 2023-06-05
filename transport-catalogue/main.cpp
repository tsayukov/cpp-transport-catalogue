#include "unit_tests.h"
#include "transport_catalogue.h"
#include "reader.h"
#include "input_reader.h"
#include "stat_reader.h"

#include<iostream>

int main() {
    using namespace transport_catalogue;

    RunAllTests();

    TransportCatalogue transport_catalogue;
    query::input::ProcessQueries(reader::GetAllQueries(std::cin), transport_catalogue);
    query::output::ProcessQueries(reader::GetAllQueries(std::cin), std::cout, transport_catalogue);
}