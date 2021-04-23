#pragma once

#include <string>
#include <grpcpp/grpcpp.h>

#include "batlshm.grpc.pb.h"

using grpc::Channel;
using std::string;

using namespace std;

enum BatlShmErrorCodes {
    OK = 0,
    FAILED = -1,
    TIMEOUT = -2
};

class BatlShmClient {
private:
    unique_ptr<BatlShm::Stub> mStub;

public:
    BatlShmClient(shared_ptr<Channel> channel);
    BatlShmClient(const string& ip="localhost", const string& port="50051");

    int32_t CreateBuffer(string& name, int32_t size);
    int32_t GetBuffer(const string& name, int32_t& size);
    int32_t ReleaseBuffer(const string& name);
    int32_t RegisterTopic(const string& name, const uint32_t size, bool wait=false);
    int32_t Publish(const string& topic_name, const string& buffer_name, uint64_t timestamp);
    int32_t Publish(const string& topic_name, const string& buffer_name, const string& metadata, uint64_t timestamp);
    int32_t GetSubscriberCount(const string& topic_name, unsigned int& num_subs);
    int32_t Subscribe(const string& topic_name, const string& subscriber_name, bool wait=false);
    int32_t Pull(const string& topic_name, const string& subscriber_name,
            string& buffer_name, uint64_t timestamp, bool block=true);
    int32_t Pull(const string& topic_name, const string& subscriber_name,
            string& buffer_name, string& metadata, uint64_t timestamp, bool block=true);
};

void* MapBuffer(const string& handle, size_t size);
void UnmapBuffer(void* memory, size_t size);
