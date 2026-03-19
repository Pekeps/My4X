set(PROTO_FILES
    resource.proto
    tile.proto
    map.proto
    building.proto
    city.proto
    unit.proto
    game_state.proto
)

# Create library with a placeholder; protobuf_generate will add the real sources.
add_library(my4x_proto STATIC ${PROTO_FILES})
target_include_directories(my4x_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(my4x_proto PUBLIC protobuf::libprotobuf absl::log_internal_check_op)

protobuf_generate(
    TARGET my4x_proto
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)
