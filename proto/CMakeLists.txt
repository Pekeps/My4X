find_package(Protobuf REQUIRED)
find_package(absl REQUIRED)

set(PROTO_FILES
    resource.proto
    tile.proto
    map.proto
    building.proto
    city.proto
    unit.proto
    game_state.proto
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(my4x_proto STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(my4x_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(my4x_proto PUBLIC protobuf::libprotobuf absl::log_internal_check_op)
