cmake_minimum_required (VERSION 2.8.11)
project(pubsub_test)

set(INC_LIST
    ${GRPC_INCLUDE_DIR}
    ${GENERATED_DIR}
    ${SHM_CLIENT_DIR})

set(SRC_LIST
    pubsub.cpp)

set(LIB_LIST
    pthread
    protobuf
    grpc
    grpc++
    rt
    shm_client)

set(LIB_FLAGS "-L${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

include_directories(${INC_LIST})
add_executable(pubsub_test ${SRC_LIST})
add_dependencies(pubsub_test shm_client)
target_link_libraries(pubsub_test ${LIB_LIST})
set_target_properties(pubsub_test PROPERTIES
    LINK_FLAGS ${LIB_FLAGS})
