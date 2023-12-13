project(shm_client)

set(LIB_LIST
    spdlog::spdlog
    proto-objects
    libprotobuf
    grpc
    grpc++
    rt
)

add_library(shm_client SHARED shm_client.cpp)
target_link_libraries(shm_client PUBLIC ${LIB_LIST})
