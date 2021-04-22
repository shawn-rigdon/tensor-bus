#include "topic_manager.h"

TopicManager* TopicManager::instance = nullptr;

TopicManager::TopicManager() {
    mActiveTopics.reserve(10);
}

bool TopicManager::addTopic(string name) {
    //TODO: revisit the return value. There's no reason to return false if it already exists
    if (mActiveTopics.find(name) != mActiveTopics.end())
        return true; // topic already exists

    mActiveTopics[name] = make_shared<Topic>(name, QUEUE_SIZE);
    return true;
}

bool TopicManager::publish(string topic_name, TopicQueueItem& item) {
    auto it = mActiveTopics.find(topic_name);
    if (it == mActiveTopics.end() || it->second->size() <= 0)
        return false; // topic has not been registered or has no subscribers

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
