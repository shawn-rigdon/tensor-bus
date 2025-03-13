cmake_minimum_required (VERSION 3.15) # required by grpc 1.60.0
set(CMAKE_CXX_STANDARD 17)

project(shm_ipc)

set(CONDA_ENV "grpc")

set(BUILD_PYTHON TRUE)
if (DEFINED NO_PYTHON_SHM)
	set(BUILD_PYTHON FALSE)
endif()
if (NOT DEFINED SHM_CLIENT_DIR)
    set(SHM_CLIENT_DIR "${CMAKE_CURRENT_LIST_DIR}/client")
endif()

option(BUILD_SHARED_LIBS "build shared libraries" ON)

# fetch dependencies
include(FetchContent)
set(JSON_Install ON)
set(SPDLOG_INSTALL ON)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
set(ABSL_PROPAGATE_CXX_STD ON)
set(ABSL_ENABLE_INSTALL ON)
FetchContent_Declare(
	gRPC
	GIT_REPOSITORY https://github.com/grpc/grpc
	GIT_TAG        v1.60.0
)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(json spdlog gRPC)


# generate proto stubs. refer to official cmake_protobuf_generate.md for explanation
include(${grpc_SOURCE_DIR}/third_party/protobuf/cmake/protobuf-generate.cmake) #TODO: find a better way to include this
add_library(proto-objects OBJECT "${CMAKE_CURRENT_LIST_DIR}/server/shm_server.proto")
set_property(TARGET proto-objects PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(proto-objects PUBLIC libprotobuf grpc++)

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(PYTHON_PROTO_BINARY_DIR "${CMAKE_CURRENT_LIST_DIR}/generated")
set(PROTO_IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}/server")
file(MAKE_DIRECTORY ${PROTO_BINARY_DIR} ${PYTHON_PROTO_BINARY_DIR})

target_include_directories(proto-objects PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")

# generate C++ stubs
protobuf_generate(
    TARGET proto-objects
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}"
)

protobuf_generate(
    TARGET proto-objects
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=\$<TARGET_FILE:grpc_cpp_plugin>"
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}"
)

# generate Python stubs
protobuf_generate(
    TARGET proto-objects
	LANGUAGE python
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
	PROTOC_OUT_DIR ${PYTHON_PROTO_BINARY_DIR}
)

protobuf_generate(
    TARGET proto-objects
    LANGUAGE grpc
    GENERATE_EXTENSIONS _pb2_grpc.py
    PLUGIN "protoc-gen-grpc=\$<TARGET_FILE:grpc_python_plugin>"
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_OUT_DIR ${PYTHON_PROTO_BINARY_DIR}
)


add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(tests)
