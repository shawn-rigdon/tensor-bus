#pragma once

#include <string>
#include <grpcpp/grpcpp.h>

#include "shm_server.grpc.pb.h"

using grpc::Channel;
using std::string;

using namespace std;

class ShmClient {
private:
    unique_ptr<Shm::Stub> mStub;

public:
    ShmClient(shared_ptr<Channel> channel);
    ShmClient(const string& ip="localhost", const string& port="50051");

    int32_t CreateBuffer(string& name, int32_t size);
    int32_t GetBuffer(const string& name, int32_t& size);
    int32_t ReleaseBuffer(const string& name);
    int32_t RegisterTopic(const string& name, bool wait=false);
    int32_t Publish(const string& topic_name, const string& buffer_name, uint64_t timestamp);
    int32_t Publish(const string& topic_name, const string& buffer_name, const string& metadata, uint64_t timestamp);
    int32_t GetSubscriberCount(const string& topic_name, unsigned int& num_subs);
    int32_t Subscribe(const string& topic_name, const string& subscriber_name, unsigned int maxQueueSize=3, bool wait=false);
    int32_t Subscribe(const string& topic_name, const string& subscriber_name, vector<string>& dependencies, unsigned int maxQueueSize=3, bool wait=false);
    int32_t Pull(const string& topic_name, const string& subscriber_name,
            string& buffer_name, uint64_t& timestamp, int timeout=-1);
    int32_t Pull(const string& topic_name, const string& subscriber_name,
            string& buffer_name, string& metadata, uint64_t& timestamp, int timeout=-1);
};

void* MapBuffer(const string& handle, size_t size);
void UnmapBuffer(void* memory, size_t size);
