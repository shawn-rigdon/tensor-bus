project(pubsub_test)

set(LIB_LIST
    pthread
    libprotobuf
    grpc
    grpc++
    rt
    shm_client)

include_directories(${SHM_CLIENT_DIR})
add_executable(pubsub_test pubsub.cpp)
target_link_libraries(pubsub_test ${LIB_LIST})
