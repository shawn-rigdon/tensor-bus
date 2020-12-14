#include "topic_manager.h"

TopicManager* TopicManager::instance = nullptr;

TopicManager::TopicManager() {
    mActiveTopics.reserve(10);
}

bool TopicManager::addTopic(string name) {
    if (mActiveTopics.find(name) != mActiveTopics.end())
        return false; // topic already exists

    mActiveTopics[name] = make_shared<Topic>(name, QUEUE_SIZE);
    return true;
}

bool TopicManager::publish(string topic_name, TopicQueueItem& item) {
    auto it = mActiveTopics.find(topic_name);
    if (it == mActiveTopics.end())
        return false; // topic has not been registered

    it->second->post(item);
    return true;
}

bool TopicManager::subscribe(string topic_name, string subscriber_name) {
    auto it = mActiveTopics.find(topic_name);
    if (it == mActiveTopics.end())
        return false; // topic doesn't exist

    return it->second->subscribe(subscriber_name);
}

bool TopicManager::pull(string topic_name,  string subscriber_name, TopicQueueItem& item) {
    auto it = mActiveTopics.find(topic_name);
    if (it == mActiveTopics.end())
        return false; // topic doesn't exist

    return it->second->pull(subscriber_name, item);
}

bool TopicManager::cancelPull(string topic_name,  string subscriber_name) {
    auto it = mActiveTopics.find(topic_name);
    if (it == mActiveTopics.end())
        return false; // topic doesn't exist

    return it->second->decIdx(subscriber_name);
}

bool TopicManager::clearOldPosts(string topic_name) {
    auto it = mActiveTopics.find(topic_name);
    if (it == mActiveTopics.end())
        return false; // topic doesn't exist

    it->second->clearProcessedPosts();
    return true;
}
