#include "shm_client.h"

#include <thread>
#include <string>

const std::string topic = "test_msgs";
const int32_t msg_size = sizeof(int);
const int msg_data = 5;

void publisher(BatlShmClient* client) {
        // register topic
        client->RegisterTopic(topic);
    
        // allocate buffer
        std::string buffer_name;
        client->CreateBuffer(buffer_name, msg_size);

        // map buffer and write data
        int* pBuf = (int*)MapBuffer(buffer_name, msg_size);
        *pBuf = 5;
        UnmapBuffer(pBuf, msg_size);

        //publish (timestamp set to 0 for test)
        client->Publish(topic, buffer_name, 0);
}

void subscriber(BatlShmClient* client) {
    const std::string subscriber_name = "image_processor";
    client->Subscribe(topic, subscriber_name);

    // pull msg
    std::string buffer_name;
    uint64_t ts;
    client->Pull(topic, subscriber_name, buffer_name, ts);

    int32_t size;
    int32_t result = client->GetBuffer(buffer_name, size);

    if (result < 0) {
        std::cerr << "GetBuffer Failed" << std::endl;
        return;
    }

    if (size != msg_size)
        std::cerr << "Received msg size does not match sent msg size" << std::endl;

    int* pBuf = (int*)MapBuffer(buffer_name, size);
    if (*pBuf == msg_data)
        std::cout << "Passed" << std::endl;
    else
        std::cerr << "Received data does not match sent data" << std::endl;

    UnmapBuffer(pBuf, size);
    client->ReleaseBuffer(buffer_name);
}

int main(int argc, char *argv[]) {
        // Get shm client
        auto ShmClient = new BatlShmClient(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
        std::thread publisher_thread(publisher, ShmClient);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::thread subscriber_thread(subscriber, ShmClient);
        publisher_thread.join();
        subscriber_thread.join();
        delete ShmClient;
}

