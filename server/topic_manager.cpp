#include "topic_manager.h"
#include "spdlog/spdlog.h"
#include <iostream>

TopicManager *TopicManager::instance = nullptr;

TopicManager::TopicManager() { mActiveTopics.reserve(10); }

bool TopicManager::addTopic(string &name, bool dropMsgs) {
  if (mActiveTopics.find(name) == mActiveTopics.end()) {
    spdlog::info("adding topic:{}", name);
    mActiveTopics[name] = make_shared<Topic>(name, dropMsgs);
  } else
    spdlog::debug("topic:{} already exists");
  return true;
}

bool TopicManager::publish(string topic_name, TopicQueueItem &item) {
  auto it = mActiveTopics.find(topic_name);
  if (it == mActiveTopics.end()) {
    spdlog::error("topic:{} has not been registered", topic_name);
    return false;
  } else if (it->second->size() <= 0) {
    spdlog::warn("topic:{} registered but no subscribers", topic_name);
    return false;
  }
  it->second->post(item);
  return true;
}

unsigned int TopicManager::getSubscriberCount(string topic_name) {
  auto it = mActiveTopics.find(topic_name);
  if (it != mActiveTopics.end())
    return it->second->size();
  else
    return 0;
}

bool TopicManager::subscribe(string topic_name, string subscriber_name,
                             std::vector<string> &dependencies,
                             unsigned int maxQueueSize) {
  auto it = mActiveTopics.find(topic_name);
  if (it == mActiveTopics.end()) {
    spdlog::error(
        "subscriber:{} cannot be added to topic:{}, topic doesn't exist",
        subscriber_name, topic_name);
    return false;
  }
  spdlog::info("adding subscriber:{} added to topic:{}", subscriber_name,
               topic_name);
  return it->second->subscribe(subscriber_name, dependencies, maxQueueSize);
}

bool TopicManager::pull(string topic_name, string subscriber_name,
                        TopicQueueItem &item, int timeout) {
  auto it = mActiveTopics.find(topic_name);
  if (it == mActiveTopics.end()) {
    spdlog::error("pull failed (subscriber:{}), topic:{} doesn't exist",
                  subscriber_name, topic_name);
    return false;
  }
  spdlog::debug("subscriber:{} pulling from topic:{}", subscriber_name,
                topic_name);
  return it->second->pull(subscriber_name, item, timeout);
}

bool TopicManager::cancelPull(string topic_name, string subscriber_name) {
  auto it = mActiveTopics.find(topic_name);
  if (it == mActiveTopics.end()) {
    spdlog::debug("topic:{} doesn't exist in active topics", topic_name);
    return false;
  }
  return it->second->decIdx(subscriber_name);
}

bool TopicManager::clearOldPosts(string topic_name, string subscriber) {
  auto it = mActiveTopics.find(topic_name);
  if (it == mActiveTopics.end()) {
    spdlog::debug("topic:{} doesn't exist in active topics", topic_name);
    return false;
  }
  it->second->clearProcessedPosts(subscriber);
  return true;
}
