project(shm_client)
add_library(shm_client SHARED shm_client.cpp)
target_link_libraries(shm_client PUBLIC spdlog::spdlog proto-objects)
