
#include <grpcpp/grpcpp.h>

#include <atomic>
#include <csignal>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex> // For std::unique_lock
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <fcntl.h> /* For O_* constants */
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <sys/types.h>
#include <unistd.h>

#include "spdlog/spdlog.h"
#include <nlohmann/json.hpp>
#include <shm_server.grpc.pb.h>

#include "shm_manager.h"
#include "topic_manager.h"

using namespace std;
using json = nlohmann::json;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class ShmServiceImpl final : public Shm::Service {
private:
  mutex mMutex;

public:
  Status CreateBuffer(ServerContext *context,
                      const CreateBufferRequest *request,
                      CreateBufferReply *reply) override {
    reply->set_result(-1);
    // create buffer name
    static std::atomic<unsigned int> count(0);
    string name = "/shmsvr_" + to_string(count++);
    spdlog::debug("allocating shm buffer {}", name);

    shared_ptr<ShmBuffer> buffer = make_shared<ShmBuffer>(name);
    if (!buffer->allocate(request->size())) {
      spdlog::error("shm buffer allocation failed for request size:{}",
                    request->size());
      return Status::CANCELLED;
    }
    ShmManager::getInstance()->add(buffer);
    reply->set_name(name);
    reply->set_result(0);
    return Status::OK;
  }

  Status GetBuffer(ServerContext *context, const GetBufferRequest *request,
                   GetBufferReply *reply) override {
    reply->set_result(-1);
    shared_ptr<ShmBuffer> buffer =
        ShmManager::getInstance()->getBuffer(request->name());
    if (buffer) {
      reply->set_size((uint32_t)buffer->getSize());
      reply->set_result(0);
    } else {
      spdlog::error("failed to get buffer:{}", request->name());
    }
    return Status::OK;
  }

  Status ReleaseBuffer(ServerContext *context,
                       const ReleaseBufferRequest *request,
                       StandardReply *reply) override {
    string name = request->name();
    ShmManager::getInstance()->release(name);
    reply->set_result(0);
    return Status::OK;
  }

  Status RegisterTopic(ServerContext *context,
                       const RegisterTopicRequest *request,
                       StandardReply *reply) override {
    reply->set_result(0);
    string name = request->name();
    TopicManager::getInstance()->addTopic(name);
    return Status::OK;
  }

  Status Publish(ServerContext *context, const PublishRequest *request,
                 StandardReply *reply) override {
    reply->set_result(-1);
    string buffer_name = request->buffer_name();
    shared_ptr<ShmBuffer> shm_buf =
        ShmManager::getInstance()->getBuffer(buffer_name);
    if (!shm_buf) {
      spdlog::error("failed to publish buffer:{}", buffer_name);
      return Status::OK; // status reply is okay, but the buffer doesn't exists
    }
    unsigned int sub_count =
        TopicManager::getInstance()->getSubscriberCount(request->topic_name());
    shm_buf->setRefCount(sub_count);
    TopicQueueItem msg(buffer_name, request->metadata(), request->timestamp());
    if (TopicManager::getInstance()->publish(request->topic_name(), msg)) {
      spdlog::debug("published buffer:{} to topic:{}", buffer_name,
                    request->topic_name());
      reply->set_result(0);
      // TODO: This should probably be handled by a client object. We don't want
      // this function
      //      to assume a buffer needs to get released.
    } else {
      spdlog::error("failed to publish buffer:{} to topic:{}", buffer_name,
                    request->topic_name());
      ShmManager::getInstance()->release(buffer_name, sub_count);
    }
    return Status::OK;
  }

  Status GetSubscriberCount(ServerContext *context,
                            const SubscriberCountRequest *request,
                            SubscriberCountReply *reply) override {
    unsigned int sub_count =
        TopicManager::getInstance()->getSubscriberCount(request->topic_name());
    reply->set_num_subs(sub_count);
    reply->set_result(0);
    return Status::OK;
  }

  //    //TODO: NEXT - figure out how to return repeated
  //    Status GetTopics(ServerContext* context, Empty* e, TopicList* tl)
  //    override {
  //    }

  Status Subscribe(ServerContext *context, const SubscribeRequest *request,
                   StandardReply *reply) override {
    reply->set_result(0);
    std::vector<string> dep;
    dep.reserve(request->dependencies_size());
    spdlog::info("Subscribe request from:{} dependencies size:{}",
                 request->subscriber_name(), request->dependencies_size());
    for (int i = 0; i < request->dependencies_size(); ++i)
      dep.emplace_back(request->dependencies(i));

    if (!TopicManager::getInstance()->subscribe(request->topic_name(),
                                                request->subscriber_name(), dep,
                                                request->maxqueuesize())) {
      spdlog::error("failed to subscribe, subscriber:{} topic:{}",
                    request->subscriber_name(), request->topic_name());
      reply->set_result(-1);
    }
    return Status::OK;
  }

  Status Pull(ServerContext *context, const PullRequest *request,
              PullReply *reply) override {
    string topic = request->topic_name();
    string subscriber = request->subscriber_name();
    int timeout = request->timeout();
    TopicQueueItem item;
    reply->set_result(-1);
    // Clear processed queue items for this set of subscribers.
    // This is done here to support client cancellation.
    TopicManager::getInstance()->clearOldPosts(topic, subscriber);
    bool gotItem =
        TopicManager::getInstance()->pull(topic, subscriber, item, timeout);
    if (!gotItem) {
      spdlog::error("failed to pull item from topic:{} subscriber:{}", topic,
                    subscriber);
      return Status::OK;
    }
    unique_lock<mutex> lock(mMutex);
    if (context->IsCancelled()) {
      spdlog::warn("context canceled, canceling pull request for topic:{} from "
                   "subscriber:{}",
                   topic, subscriber);
      TopicManager::getInstance()->cancelPull(topic, subscriber);
      return Status::CANCELLED;
    }
    if (!item.buffer_name.empty()) {
      reply->set_result(0);
      reply->set_buffer_name(item.buffer_name);
      reply->set_metadata(item.metadata);
      reply->set_timestamp(item.timestamp);
      spdlog::debug("pulling buffer:{} from topic:{} by subscriber:{}",
                    item.buffer_name, topic, subscriber);
    } else
      spdlog::error("buffer_name is empty");
    return Status::OK;
  }
};

void RunServer(std::string port) {
  spdlog::info("launching shm_server on port:{}", port);
  std::string server_address("0.0.0.0:" + port);
  ShmServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

void SignalHandler(int signum) {
  ShmManager::getInstance()->releaseAll();
  exit(signum);
}

inline bool file_exists(const std::string &name) {
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

template <typename PARAM_T>
bool get_json_param(const json &j, const std::string &p_name, PARAM_T &var) {
  try {
    auto val = j.at(p_name).get<PARAM_T>();
    var = val;
    return true;
  } catch (const std::exception &e) {
    return false;
  }
}

int main(int argc, char **argv) {
  std::signal(SIGINT, SignalHandler); // release memory if server is terminated

  json server_params;
  std::string log_level = "error", port = "50051";
  // Read the config file if provided to initialize the server
  if (argc > 1) {
    if (not file_exists(argv[1]))
      throw std::invalid_argument("Application config file \"" +
                                  std::string(argv[1]) + "\" does not exist.");
    try {
      std::ifstream ifs(argv[1]);
      ifs >> server_params;
    } catch (const std::exception &e) {
      throw std::invalid_argument(
          "Cannot read application parameters file: \"" + std::string(argv[1]) +
          "\".");
    }

    // read arguments from config file
    get_json_param(server_params, std::string("log_level"), log_level);
    get_json_param(server_params, std::string("port"), port);
  }

  // set the log level from the config
  if (!log_level.compare("error"))
    spdlog::set_level(spdlog::level::err);
  else if (!log_level.compare("info"))
    spdlog::set_level(spdlog::level::info);
  else
    spdlog::set_level(spdlog::level::debug);
  
  RunServer(port);
  return 0;
}
