project(shm_client)

# generated names
set(PB_SRC "${GENERATED_DIR}/batlshm.pb.cc")
set(GRPC_SRC "${GENERATED_DIR}/batlshm.grpc.pb.cc")
set_source_files_properties(${PB_SRC} PROPERTIES GENERATED TRUE)
set_source_files_properties(${GRPC_SRC} PROPERTIES GENERATED TRUE)

set(LIB_LIST
    protobuf
    rt)

set(SRC_LIST shm_client.cpp ${GRPC_SRC} ${PB_SRC})
set(LIB_FLAGS "-L${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
include_directories(${GENERATED_DIR} ${GRPC_INCLUDE_DIR} ${PROTO_SRC})
#add_library(shm_client ${SRC_LIST})
add_library(shm_client SHARED ${SRC_LIST})
set_property(TARGET shm_client PROPERTY LINK_FLAGS ${LIB_FLAGS})
target_link_libraries(shm_client ${LIB_LIST})
#set_property(TARGET shm_client PROPERTY INTERFACE_LINK_LIBRARIES protobuf)
add_dependencies(shm_client shm_server)
