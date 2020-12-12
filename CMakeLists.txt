cmake_minimum_required (VERSION 2.8.11)
project(shm_ipc)

#if (NOT DEFINED GENERATED_DIR)
#    set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
#endif()

set(GRPC_INSTALL_DIR "${CMAKE_SOURCE_DIR}/lib/grpc")
set(GRPC_INCLUDE_DIR "${GRPC_INSTALL_DIR}/include")
set(GRPC_LIB_DIR "${GRPC_INSTALL_DIR}/lib")

add_subdirectory (server)
add_subdirectory (client)
