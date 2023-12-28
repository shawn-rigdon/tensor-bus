#pragma once

#include <unordered_map>

#include "topic_queue.h"

// There should never be a need to have a queue size greater than 3 for
// all topics.
// TODO: what happens if 3 topics in the pipeline have different queue sizes?
//#define DEFAULT_QUEUE_SIZE  3

class TopicManager {
private:
  static TopicManager *instance;
  unordered_map<string, shared_ptr<Topic>> mActiveTopics;

  TopicManager();

public:
  static TopicManager *getInstance() {
    if (!instance)
      instance = new TopicManager();
    return instance;
  }

  bool addTopic(string &name, bool dropMsgs=false);
  bool publish(string topic_name, TopicQueueItem &item);
  bool subscribe(string topic_name, string subscriber_name,
                 std::vector<string> &dependencies, unsigned int maxQueueSize);
  bool pull(string topic_name, string subscriber_name, TopicQueueItem &item,
            int timeout = -1);
  bool cancelPull(string topic_name, string subscriber_name);
  bool clearOldPosts(string topic_name, string subscriber_name);
  unsigned int getSubscriberCount(string topic_name);

  ~TopicManager() { delete instance; }
};
