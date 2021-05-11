#include "topic_manager.h"

#include <iostream>

TopicManager* TopicManager::instance = nullptr;

TopicManager::TopicManager() {
    mActiveTopics.reserve(10);
}

bool TopicManager::addTopic(string& name) {
std::cout << "Adding topic: " << name << std::endl;
    //TODO: revisit the return value. There's no reason to return false if it already exists
    if (mActiveTopics.find(name) != mActiveTopics.end())
{
std::cout << "nope" << std::endl;
        return true; // topic already exists
}

    mActiveTopics[name] = make_shared<Topic>(name);
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

bool TopicManager::subscribe(string topic_name, string subscriber_name, std::vector<string>& dependencies, unsigned int maxQueueSize) {
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
std::cout << "topic: " << topic_name << std::endl;
    auto it = mActiveTopics.find(topic_name);
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    if (it == mActiveTopics.end())
{
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        return false; // topic doesn't exist
}

std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    return it->second->subscribe(subscriber_name, dependencies, maxQueueSize);
}

bool TopicManager::pull(string topic_name,  string subscriber_name, TopicQueueItem& item, int timeout) {
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    auto it = mActiveTopics.find(topic_name);
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    if (it == mActiveTopics.end())
{
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        return false; // topic doesn't exist
}

std::cout << __FILE__ << ": " << __LINE__ << std::endl;
std::cout << "Pulling topic: " << topic_name << std::endl;
    return it->second->pull(subscriber_name, item, timeout);
}

bool TopicManager::cancelPull(string topic_name,  string subscriber_name) {
    auto it = mActiveTopics.find(topic_name);
    if (it == mActiveTopics.end())
        return false; // topic doesn't exist

    return it->second->decIdx(subscriber_name);
}

bool TopicManager::clearOldPosts(string topic_name, string subscriber) {
    auto it = mActiveTopics.find(topic_name);
    if (it == mActiveTopics.end())
        return false; // topic doesn't exist

    it->second->clearProcessedPosts(subscriber);
    return true;
}
