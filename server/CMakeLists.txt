project(shm_server)

#set(GRPC_BIN_DIR "${GRPC_INSTALL_DIR}/bin")
set(PROTOC "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/protoc")
set(CPP_PLUGIN "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/grpc_cpp_plugin")

# generated names
set(PB_SRC "${GENERATED_DIR}/batlshm.pb.cc")
set(PB_HDR "${GENERATED_DIR}/batlshm.pb.h")
set(GRPC_SRC "${GENERATED_DIR}/batlshm.grpc.pb.cc")
set(GRPC_HDR "${GENERATED_DIR}/batlshm.grpc.pb.h")
set(GENERATED_FILES ${PB_SRC} ${PB_HDR} ${GRPC_SRC} ${GRPC_HDR})

# cpp generated files
add_custom_command(
    OUTPUT ${GENERATED_FILES}
    COMMAND mkdir
    ARGS -p "${GENERATED_DIR}"
    COMMAND ${PROTOC}
    ARGS --grpc_out "${GENERATED_DIR}"
        --cpp_out "${GENERATED_DIR}"
        --plugin=protoc-gen-grpc="${CPP_PLUGIN}"
        -I ${CMAKE_CURRENT_SOURCE_DIR}
        -I ${GRPC_INCLUDE_DIR}
        batlshm.proto
    DEPENDS batlshm.proto)

#set(GENERATED_PYTHON ${GENERATED_DIR}/batlshm_pb2_grpc.py ${GENERATED_DIR}/batlshm_pb2.py)
#add_custom_command(
#    OUTPUT ${GENERATED_PYTHON}
#    COMMAND mkdir
#    ARGS -p "${GENERATED_DIR}"
#    COMMAND python
#    ARGS -m grpc.tools.protoc
#        -I ${CMAKE_CURRENT_SOURCE_DIR}
#        -I ${GRPC_INCLUDE_DIR}
#        --python_out=${GENERATED_DIR}
#        --grpc_python_out=${GENERATED_DIR}
#        batlshm.proto
#    DEPENDS batlshm.proto)
#
#add_custom_target(
#    generate_python ALL
#    DEPENDS ${GENERATED_PYTHON})

set(SRC_LIST shm_server.cpp topic_manager.cpp topic_queue.cpp shm_manager.cpp)
set(LD_FLAGS "-L${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(LD_LIBS "-lprotobuf -lgrpc++ -lgrpc -lrt")

include_directories(${GENERATED_DIR} ${GRPC_INCLUDE_DIR} ${PROTO_SRC})
add_executable(shm_server ${SRC_LIST} ${PB_SRC} ${GRPC_SRC})
set_target_properties(shm_server PROPERTIES
    LINK_FLAGS ${LD_FLAGS}
    LINK_LIBRARIES ${LD_LIBS})
add_dependencies(shm_server grpc spdlog::spdlog nlohmann_json::nlohmann_json)
target_link_libraries(shm_server PRIVATE spdlog::spdlog nlohmann_json::nlohmann_json)
