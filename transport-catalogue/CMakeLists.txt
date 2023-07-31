cmake_minimum_required(VERSION 3.18)

project(TransportCatalogue CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
        transport_catalogue.proto
        map_renderer.proto
        svg.proto
        transport_router.proto
        graph.proto)

set(TEST_FILES
        log_duration.h log_duration.cpp
        tests/unit_test_tools.h
        tests/unit_tests.h)

set(INNER_LIBRARY_FILES
        geo.h geo.cpp
        number_wrapper.h
        kahan_algorithm.h
        str_view_handler.h str_view_handler.cpp
        json.h json.cpp
        json_builder.h json_builder.cpp
        svg.h svg.cpp
        ranges.h
        graph.h
        router.h)

set(DOMAIN_FILES domain.h domain.cpp)

set(REQUEST_HANDLER_FILES
        queries.h queries.cpp
        reader.h reader.cpp
        input_reader.h input_reader.cpp
        stat_reader.h stat_reader.cpp
        json_reader.h json_reader.cpp
        request_handler.h request_handler.cpp)

set(SERIALIZATION_FILES serialization.h serialization.cpp)

set(DATABASE_FILES
        transport_catalogue.h transport_catalogue.cpp
        map_renderer.h map_renderer.cpp
        transport_router.h transport_router.cpp)

add_executable(transport_catalogue
        main.cpp
        ${PROTO_SRCS} ${PROTO_HDRS}
        ${TEST_FILES}
        ${INNER_LIBRARY_FILES}
        ${DOMAIN_FILES}
        ${REQUEST_HANDLER_FILES}
        ${SERIALIZATION_FILES}
        ${DATABASE_FILES})

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue
        "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>"
        Threads::Threads)