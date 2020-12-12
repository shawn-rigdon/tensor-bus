#include "topic_manager.h"

TopicManager* TopicManager::instance = nullptr;

TopicManager::TopicManager() {
    nextID = 0;
    mActiveTopics.reserve(10);
}

SubscriberID TopicManager::createID() {
    return nextID++;
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

bool TopicManager::subscribe(string topic_name, SubscriberID id) {
    auto it = mActiveTopics.find(topic_name);
    if (it == mActiveTopics.end())
        return false; // topic doesn't exist

    return it->second->subscribe(id);
}

bool TopicManager::pull(string topic_name, SubscriberID id, TopicQueueItem& item) {
    auto it = mActiveTopics.find(topic_name);
    if (it == mActiveTopics.end())
        return false; // topic doesn't exist

    return it->second->pull(id, item);
}

bool TopicManager::cancelPull(string topic_name, SubscriberID id) {
    auto it = mActiveTopics.find(topic_name);
    if (it == mActiveTopics.end())
        return false; // topic doesn't exist

    return it->second->decIdx(id);
}

bool TopicManager::clearOldPosts(string topic_name) {
    auto it = mActiveTopics.find(topic_name);
    if (it == mActiveTopics.end())
        return false; // topic doesn't exist

    it->second->clearProcessedPosts();
    return true;
}
