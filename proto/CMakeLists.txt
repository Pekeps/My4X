find_package(Protobuf REQUIRED)

# --------------------------------------------------------------------------
# Message-only proto files (no gRPC service definitions).
# --------------------------------------------------------------------------
set(PROTO_FILES
    common.proto
    resource.proto
    tile.proto
    map.proto
    building.proto
    city.proto
    unit.proto
    game_state.proto
    game_action.proto
    game_event.proto
    lobby.proto
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(my4x_proto STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(my4x_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(my4x_proto PUBLIC protobuf::libprotobuf)

# Protobuf v22+ depends on abseil internally; link it when available.
find_package(absl QUIET)
if(absl_FOUND)
    target_link_libraries(my4x_proto PUBLIC absl::log_internal_check_op)
endif()

# --------------------------------------------------------------------------
# gRPC service proto files — generates both protobuf and gRPC C++ stubs.
# Only built when gRPC is available on the system.
# --------------------------------------------------------------------------
find_package(gRPC CONFIG QUIET)
if(gRPC_FOUND)
    message(STATUS "gRPC found — building gRPC service stubs")

    set(GRPC_PROTO_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/lobby_service.proto
        ${CMAKE_CURRENT_SOURCE_DIR}/game_service.proto
    )

    # Locate the gRPC C++ plugin and protoc compiler.
    get_target_property(_GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
    get_target_property(_PROTOC protobuf::protoc LOCATION)

    set(GRPC_SRCS)
    set(GRPC_HDRS)
    set(GRPC_PB_SRCS)
    set(GRPC_PB_HDRS)

    foreach(_proto ${GRPC_PROTO_FILES})
        get_filename_component(_proto_name ${_proto} NAME_WE)

        set(_pb_cc  "${CMAKE_CURRENT_BINARY_DIR}/${_proto_name}.pb.cc")
        set(_pb_h   "${CMAKE_CURRENT_BINARY_DIR}/${_proto_name}.pb.h")
        set(_grpc_cc "${CMAKE_CURRENT_BINARY_DIR}/${_proto_name}.grpc.pb.cc")
        set(_grpc_h  "${CMAKE_CURRENT_BINARY_DIR}/${_proto_name}.grpc.pb.h")

        add_custom_command(
            OUTPUT ${_pb_cc} ${_pb_h} ${_grpc_cc} ${_grpc_h}
            COMMAND ${_PROTOC}
                --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
                --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
                --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
                --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN}
                ${_proto}
            DEPENDS ${_proto} ${PROTO_FILES}
            COMMENT "Generating gRPC stubs for ${_proto_name}.proto"
        )

        list(APPEND GRPC_PB_SRCS ${_pb_cc})
        list(APPEND GRPC_PB_HDRS ${_pb_h})
        list(APPEND GRPC_SRCS ${_grpc_cc})
        list(APPEND GRPC_HDRS ${_grpc_h})
    endforeach()

    add_library(my4x_grpc STATIC
        ${GRPC_SRCS} ${GRPC_HDRS}
        ${GRPC_PB_SRCS} ${GRPC_PB_HDRS}
    )
    target_include_directories(my4x_grpc PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(my4x_grpc PUBLIC my4x_proto gRPC::grpc++)
else()
    message(STATUS "gRPC not found — skipping gRPC service stubs (install libgrpc++-dev to enable)")
endif()
