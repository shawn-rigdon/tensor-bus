#pragma once

#include <unordered_map>

#include "topic_queue.h"

// There should never be a need to have a queue size greater than 2 for
// all topics.
// TODO: what happens if 2 topics in the pipeline have different queue sizes?
#define QUEUE_SIZE  2

class TopicManager {
private:
    static TopicManager* instance;
    unordered_map< string, shared_ptr<Topic> > mActiveTopics;
    SubscriberID nextID;

    TopicManager();

public:
    static TopicManager* getInstance() {
        if (!instance)
            instance = new TopicManager();
        return instance;
    }

    SubscriberID createID();
    bool addTopic(string name);
    bool publish(string topic_name, TopicQueueItem& item);
    bool subscribe(string topic_name, SubscriberID id);
    bool pull(string topic_name, SubscriberID id, TopicQueueItem& item);
    bool cancelPull(string topic_name, SubscriberID);
    bool clearOldPosts(string topic_name);

    ~TopicManager() {delete instance;}
};
