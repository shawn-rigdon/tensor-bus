project(shm_server)

set(SRC_LIST
	shm_server.cpp
	topic_manager.cpp
	topic_queue.cpp
	shm_manager.cpp
)

set(LD_LIBS
	spdlog::spdlog
	nlohmann_json::nlohmann_json
	proto-objects
	libprotobuf
	grpc++
	grpc
	gpr
	rt
)

add_executable(shm_server ${SRC_LIST})
target_link_libraries(shm_server PUBLIC ${LD_LIBS})
