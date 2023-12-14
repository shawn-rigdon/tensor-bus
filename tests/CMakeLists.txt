project(pubsub_test)
include_directories(${SHM_CLIENT_DIR})
add_executable(pubsub_test pubsub.cpp)
target_link_libraries(pubsub_test shm_client)
