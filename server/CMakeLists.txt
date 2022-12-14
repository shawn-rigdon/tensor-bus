project(shm_server)

#set(GRPC_BIN_DIR "${GRPC_INSTALL_DIR}/bin")
set(PROTOC "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/protoc")
set(CPP_PLUGIN "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/grpc_cpp_plugin")

# generated names
set(PB_SRC "${GENERATED_DIR}/shm_server.pb.cc")
set(PB_HDR "${GENERATED_DIR}/shm_server.pb.h")
set(GRPC_SRC "${GENERATED_DIR}/shm_server.grpc.pb.cc")
set(GRPC_HDR "${GENERATED_DIR}/shm_server.grpc.pb.h")
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
        shm_server.proto
    DEPENDS shm_server.proto)

if (DEFINED BUILD_PYTHON)
	message(STATUS "Building Python proto stubs")
	execute_process(
		COMMAND which conda
		OUTPUT_VARIABLE TMP # using to keep output from dumping to console
		RESULT_VARIABLE RES)
	if (NOT ${RES} EQUAL 0)
		message(SEND_ERROR "You must install Conda on your system to build the Python stubs")
	endif()
	execute_process(
		COMMAND conda env list
		OUTPUT_VARIABLE ENVS)
	if (NOT ${ENVS} MATCHES ${CONDA_ENV})
		message(STATUS "Creating ${CONDA_ENV} conda environment")
		set(ENV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/environment.yml")
		execute_process(COMMAND conda env update -f "${ENV_FILE}")
	endif()
	set(PYTHON_GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../generated")
	execute_process( # build stubs
		COMMAND mkdir -p ${PYTHON_GENERATED_DIR}
		COMMAND conda run -n grpc python -m grpc.tools.protoc -I${CMAKE_CURRENT_SOURCE_DIR}
		--python_out=${PYTHON_GENERATED_DIR} --grpc_python_out=${PYTHON_GENERATED_DIR} shm_server.proto)
endif()

set(SRC_LIST shm_server.cpp topic_manager.cpp topic_queue.cpp shm_manager.cpp)
set(LD_FLAGS "-L${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(LD_LIBS "-lprotobuf -lgrpc++ -lgrpc -lgpr -labsl_synchronization -lrt")

include_directories(${GENERATED_DIR} ${GRPC_INCLUDE_DIR} ${ABSEIL_CPP_DIR} ${PROTO_SRC})
add_executable(shm_server ${SRC_LIST} ${PB_SRC} ${GRPC_SRC})
set_target_properties(shm_server PROPERTIES
    LINK_FLAGS ${LD_FLAGS}
    LINK_LIBRARIES ${LD_LIBS})
add_dependencies(shm_server grpc spdlog::spdlog nlohmann_json::nlohmann_json)
target_link_libraries(shm_server PRIVATE spdlog::spdlog nlohmann_json::nlohmann_json)
