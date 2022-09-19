cmake_minimum_required (VERSION 3.10)
set(CMAKE_CXX_STANDARD 14)

project(shm_ipc)

set(CONDA_ENV "grpc")

set(BUILD_PYTHON TRUE)
if (DEFINED NO_PYTHON_SHM)
	set(BUILD_PYTHON FALSE)
endif()
if (NOT DEFINED GENERATED_DIR)
    set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
endif()
if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
endif()
if (NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
endif()
if (NOT DEFINED SHM_CLIENT_DIR)
    set(SHM_CLIENT_DIR "${CMAKE_CURRENT_LIST_DIR}/client")
endif()

option(BUILD_SHARED_LIBS "build shared libraries" ON)

#add_subdirectory("${CMAKE_SOURCE_DIR}/dependencies/grpc")
if (NOT DEFINED GRPC_INSTALL_DIR)
    set(GRPC_INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/dependencies/grpc")
    set(PROTO_SRC "${GRPC_INSTALL_DIR}/third_party/protobuf/src")
endif()
add_subdirectory(${GRPC_INSTALL_DIR})

# search for required spdlog version and build if not found
if (NOT ${SEARCHED_SPDLOG})
	include("${CMAKE_CURRENT_LIST_DIR}/cmake/find_spdlog.cmake")
	find_spdlog(${CMAKE_CURRENT_LIST_DIR})
endif()

if (NOT DEFINED JSON_SRC_DIR)
    set(JSON_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/dependencies/json")
endif()
add_subdirectory(${JSON_SRC_DIR})

#set(GRPC_INSTALL_DIR "${CMAKE_SOURCE_DIR}/lib/grpc")
set(GRPC_INCLUDE_DIR "${GRPC_INSTALL_DIR}/include")
#set(GRPC_LIB_DIR "${GRPC_INSTALL_DIR}/lib")

#set(GRPC_INCLUDE_DIR ${GRPC_INSTALL_DIR}/${gRPC_INSTALL_INCLUDEDIR})

add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(tests)
