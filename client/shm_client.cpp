#include "shm_client.h"

#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <grpcpp/grpcpp.h>

#include "spdlog/spdlog.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using std::string;

BatlShmClient::BatlShmClient(std::shared_ptr<Channel> channel) :
    mStub(BatlShm::NewStub(channel))
{
}

int32_t BatlShmClient::CreateBuffer(string& name, int32_t size) {
    CreateBufferRequest request;
    CreateBufferReply reply;
    ClientContext context;
    request.set_size(size);
    Status status = mStub->CreateBuffer(&context, request, &reply);
    if (status.ok() && !reply.name().empty())
        name = reply.name();
    else {
//        spdlog::warn("CreateBuffer() fails with error code: {}, error message: {}",
//                status.error_code(), status.error_message());
    }
    return reply.result();
}

int32_t BatlShmClient::GetBuffer(const string& name, int32_t& size) {
    GetBufferRequest request;
    GetBufferReply reply;
    ClientContext context;
    request.set_name(name);
    Status status = mStub->GetBuffer(&context, request, &reply);
    if (status.ok() && reply.result() == 0)
        size = reply.size();
    else {
//        spdlog::warn("GetBuffer() fails with error code: {}, error message: {}",
//                status.error_code(), status.error_message());
    }
    return reply.result();
}

int32_t BatlShmClient::ReleaseBuffer(const string& name) {
    ReleaseBufferRequest request;
    StandardReply reply;
    ClientContext context;
    request.set_name(name);
    Status status = mStub->ReleaseBuffer(&context, request, &reply);
    if (status.ok())
        return reply.result();

//    spdlog::warn("ReleaseBuffer() fails with error code: {}, error message: {}",
//            status.error_code(), status.error_message());
    return -1;
}

int32_t BatlShmClient::RegisterTopic(const string& name) {
    RegisterTopicRequest request;
    StandardReply reply;
    ClientContext context;
    request.set_name(name);
    Status status = mStub->RegisterTopic(&context, request, &reply);
    if (status.ok())
        return reply.result();

//    spdlog::warn("RegisterTopic() fails with error code: {}, error message: {}",
//            status.error_code(), status.error_message());
    return -1;
}

int32_t BatlShmClient::Publish(const string& topic_name,
        const string& buffer_name, uint64_t timestamp) {
    PublishRequest request;
    StandardReply reply;
    ClientContext context;
    request.set_topic_name(topic_name);
    request.set_buffer_name(buffer_name);
    request.set_timestamp(timestamp);
    Status status = mStub->Publish(&context, request, &reply);
    if (status.ok())
        return reply.result();

//    spdlog::warn("Publish() fails with error code: {}, error message: {}",
//            status.error_code(), status.error_message());
    return -1;
}

int32_t BatlShmClient::GetSubscriberCount(const string& topic_name, unsigned int& num_subs) {
    SubscriberCountRequest request;
    SubscriberCountReply reply;
    ClientContext context;
    request.set_topic_name(topic_name);
    Status status = mStub->GetSubscriberCount(&context, request, &reply);
    if (status.ok() && reply.result() == 0)
        num_subs = reply.num_subs();

    return reply.result();
}

int32_t BatlShmClient::Subscribe(const string& topic_name, const string& subscriber_name) {
    SubscribeRequest request;
    StandardReply reply;
    ClientContext context;
    request.set_topic_name(topic_name);
    request.set_subscriber_name(subscriber_name);
    Status status = mStub->Subscribe(&context, request, &reply);
    if (status.ok())
        return reply.result();

//    spdlog::warn("Subscribe() fails with error code: {}, error message: {}",
//            status.error_code(), status.error_message());
    return -1;
}

int32_t BatlShmClient::Pull(const string& topic_name, const string& subscriber_name,
        string& buffer_name, uint64_t timestamp) {
    PullRequest request;
    PullReply reply;
    ClientContext context;
    request.set_topic_name(topic_name);
    request.set_subscriber_name(subscriber_name);
    Status status = mStub->Pull(&context, request, &reply);
    if (status.ok()) {
        if (reply.result() == 0) {
            buffer_name = reply.buffer_name();
            timestamp = reply.timestamp();
            return 0;
        }

//        spdlog::info("Pull() timed out");
    }

//    spdlog::warn("Pull() fails with error code: {}, error message: {}",
//            status.error_code(), status.error_message());
    return -1;
}

void* MapBuffer(const string& name, size_t size) {
    int fd;
    void* addr = nullptr;
    int result = shm_open(name.c_str(), O_RDWR, 0);
    if ( result > 0 ){
        fd = result;
        addr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
    }
    return addr;
}

void UnmapBuffer(void* memory, size_t size) {
    munmap(memory, size);
}
