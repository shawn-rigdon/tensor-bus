
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>  // For std::unique_lock
#include <shared_mutex>
#include <csignal>

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <sys/types.h>

#include <batlshm.grpc.pb.h>
#include "spdlog/spdlog.h"

#include "shm_manager.h"
#include "topic_manager.h"

using namespace std;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class BatlShmServiceImpl final : public BatlShm::Service {
private:
    mutex mMutex;

public:
    Status CreateBuffer(ServerContext* context, const CreateBufferRequest* request,
            CreateBufferReply* reply) override {
        reply->set_result(-1);

        // create buffer name
        static std::atomic<unsigned int> count(0);
        string name = "/batl_" + to_string(count++);

        shared_ptr<ShmBuffer> buffer = make_shared<ShmBuffer>(name);
        if ( !buffer->allocate(request->size()) )
            return Status::CANCELLED;

        ShmManager::getInstance()->add(buffer);
        reply->set_name(name);
        reply->set_result(0);
        return Status::OK;
    }

    Status GetBuffer(ServerContext* context, const GetBufferRequest* request,
            GetBufferReply* reply) override {
        reply->set_result(-1);
        shared_ptr<ShmBuffer> buffer = ShmManager::getInstance()->getBuffer(request->name());
        if (buffer) {
            reply->set_size((uint32_t)buffer->getSize());
            reply->set_result(0);
        }

        return Status::OK;
    }

    Status ReleaseBuffer(ServerContext* context, const ReleaseBufferRequest* request,
            StandardReply* reply) override {
        string name = request->name();
        ShmManager::getInstance()->release(name);
        reply->set_result(0);
        return Status::OK;
    }

    Status RegisterTopic(ServerContext* context, const RegisterTopicRequest* request,
            StandardReply* reply) override {
        reply->set_result(0);
        if (!TopicManager::getInstance()->addTopic(request->name()))
            reply->set_result(-1); // topic already exists

        return Status::OK;
    }

    Status Publish(ServerContext* context, const PublishRequest* request,
            StandardReply* reply) override {
        reply->set_result(-1);
        string buffer_name = request->buffer_name();
        shared_ptr<ShmBuffer> shm_buf = ShmManager::getInstance()->getBuffer(buffer_name);
        if (!shm_buf)
            return Status::OK;

        TopicQueueItem msg(request->buffer_name(), request->timestamp());
        if (TopicManager::getInstance()->publish(request->topic_name(), msg))
            reply->set_result(0);
        //TODO: This should probably be handled by a client object. We don't want this function
        //      to assume a buffer needs to get released.
        else
            ShmManager::getInstance()->release(buffer_name);

        return Status::OK;
    }

    Status GetSubscriberCount(ServerContext* context, const SubscriberCountRequest* request,
            SubscriberCountReply* reply) override {
        unsigned int sub_count = TopicManager::getInstance()->getSubscriberCount(request->topic_name());
        reply->set_num_subs(sub_count);
        reply->set_result(0);
        return Status::OK;
    }

//    //TODO: NEXT - figure out how to return repeated
//    Status GetTopics(ServerContext* context, Empty* e, TopicList* tl) override {
//    }

    Status Subscribe(ServerContext* context, const SubscribeRequest* request,
            StandardReply* reply) override {
        reply->set_result(0);
        if (!TopicManager::getInstance()->subscribe(request->topic_name(), request->subscriber_name()))
            reply->set_result(-1);

        return Status::OK;
    }

    Status Pull(ServerContext* context, const PullRequest* request,
            PullReply* reply) override {
        string topic = request->topic_name();
        string subscriber = request->subscriber_name();
        TopicQueueItem item;
        reply->set_result(-1);
        bool gotItem = TopicManager::getInstance()->pull(topic, subscriber, item);
        if (!gotItem)
            return Status::CANCELLED;

        unique_lock<mutex> lock(mMutex);
        if (context->IsCancelled()) {
            TopicManager::getInstance()->cancelPull(topic, subscriber);
            return Status::CANCELLED;
        }

        // Got a topic item, so we need to release old items before returning.
        // This is done here to support client cancellation.
        TopicManager::getInstance()->clearOldPosts(topic);

        if (!item.buffer_name.empty()) {
            reply->set_result(0);
            reply->set_buffer_name(item.buffer_name);
            reply->set_timestamp(item.timestamp);
        }

        return Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    BatlShmServiceImpl service;

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr < Server > server(builder.BuildAndStart());

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

void SignalHandler(int signal) {
    ShmManager::getInstance()->releaseAll();
    exit(signal);
}

int main(int argc, char** argv) {
    std::signal(SIGINT, SignalHandler); // release memory if server is terminated
    spdlog::set_level(spdlog::level::debug);
    RunServer();
    return 0;
}
