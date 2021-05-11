#include "topic_queue.h"
#include "shm_manager.h"

#include <iostream>

TopicQueueItem::TopicQueueItem(const string& name, const string& metadata, const uint64_t ts) :
    buffer_name(name),
    metadata(metadata),
    timestamp(ts)
{
}

TopicQueue::TopicQueue(unsigned int maxQueueSize):
    mMaxQueueSize(maxQueueSize)
{
}

Topic::Topic(string name) :
    mName(name)
{
}

// If the subscriber is not already subscribed to the topic
// the given subscriber name is set to the oldest position in the queue.
// This subscriber method allows multiple subscribers of the same name.
bool Topic::subscribe(string& subscriber_name, std::vector<string>& dependencies, unsigned int maxQueueSize) {
    unique_lock<mutex> lock(mMutex);
    if (dependencies.size() > 0) {
        if (dependencyMap.find(subscriber_name) != dependencyMap.end())
            return true;

        std::cout << "Searching for dependency" << std::endl;
        string foundName;
        bool foundDependency = false;
        while (!foundDependency) {
            for (int i=0; i < dependencies.size(); ++i) {
                if (mQueueMap.find(dependencies[i]) != mQueueMap.end()) {
                    foundDependency = true;
                    foundName = dependencies[i];
                    break;
                }
            }
            mCV.wait(lock);
        }
        dependencyMap[subscriber_name] = foundName; // point to a parent queue
        mQueueMap[foundName]->mIndexMap[subscriber_name] = 0; // init position index in parent queue
    } else if (mQueueMap.find(subscriber_name) == mQueueMap.end()) {
        mQueueMap[subscriber_name] = make_shared<TopicQueue>(maxQueueSize);
        mQueueMap[subscriber_name]->mIndexMap[subscriber_name] = 0;
        mCV.notify_all();
    }

    return true;
}

void Topic::post(TopicQueueItem& item) {
    unique_lock<mutex> lock(mMutex);
int loopCnt = 0;
    for (auto q_it = mQueueMap.begin(); q_it != mQueueMap.end(); ++q_it) {
std::cout << "loopcnt: " << ++loopCnt << std::endl;
        shared_ptr<TopicQueue> q = q_it->second; 
std::cout << "Subsriber (" << q_it->first << ") queue size: " << q->size() << std::endl;
        if (q->mMaxQueueSize == 0 || q->size() < q->mMaxQueueSize) {
std::cout << "Posting buffer: " << item.buffer_name << std::endl;
            q->push(item);
            q->cv_idx.notify_all();
            continue;
        }

        // Remove the oldest queue element not currently being processed by
        // a subscriber. This keeps the topic up to date.
        // Oldest free topic is at the max subscriber index + 1

        unsigned int maxIdx = 0;
        for (auto it = q->mIndexMap.begin(); it != q->mIndexMap.end(); it++) {
            if (it->second > maxIdx)
                maxIdx = it->second;
        }

std::cout << "max idx: " << maxIdx << std::endl;
        int removeIdx = maxIdx + 1;
        if (removeIdx < q->mMaxQueueSize) { // mMaxQueueSize = mQueue.size() if this block is executed
            TopicQueueItem removeItem;
            q->get_val_by_index(removeItem, removeIdx);
std::cout << "POST: releasing " << removeItem.buffer_name << std::endl;
            ShmManager::getInstance()->release(removeItem.buffer_name, q->mIndexMap.size());
            q->erase(removeIdx);
            q->push(item);
        }
    }
}

// this should set item according to the subscriber id's index, increment the index,
// and pop any elements that have been seen by all subscribers. If the current index
// is greater than the number of queue elements, block until data is available by default.
// If block false immediately return when there is no available data
bool Topic::pull(string& subscriber_name, TopicQueueItem& item, int timeout) {
std::cout << "Pulling for subscriber: " << subscriber_name << std::endl;
std::cout << "timeout: " << timeout << std::endl;
    shared_ptr<TopicQueue> q;
    if (dependencyMap.find(subscriber_name) != dependencyMap.end())
{
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        q = mQueueMap[ dependencyMap[subscriber_name] ];
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
}
    else
{
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        q = mQueueMap[subscriber_name];
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
}

    unique_lock<mutex> lock(q->mutex_idx);
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    auto it = q->mIndexMap.find(subscriber_name);
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    if (it == q->mIndexMap.end()) {
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        //spdlog::error("Subscriber ID {} is not assigned to topic {}", id, mName);
        return false;
    }

    // If the current subscriber has processed all available queue messages,
    // it should wait for other subscribers to free up old messages and/or
    // the publisher to post new data
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    while (it->second >= q->size()) { // it->second retrieves current subscriber index
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        //spdlog::debug("Subscriber {} is waiting for new data", id);
        if (timeout < 0)
{
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            q->cv_idx.wait(lock); // block until new data is available
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
}
        else {
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            auto status = q->cv_idx.wait_until(lock, system_clock::now() + timeout*1ms);
std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            if (status == std::cv_status::timeout)
                return false; // return false if timeout
        }
    }

std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    q->get_val_by_index(item, it->second++); // note the index is incremented
    return true;
}

bool Topic::decIdx(string& subscriber_name) {
    shared_ptr<TopicQueue> q;
    if (dependencyMap.find(subscriber_name) != dependencyMap.end())
        q = mQueueMap[ dependencyMap[subscriber_name] ];
    else
        q = mQueueMap[subscriber_name];

    unique_lock<mutex> lock(q->mutex_idx);
    auto it = q->mIndexMap.find(subscriber_name);
    if (it == q->mIndexMap.end()) {
        //spdlog::error("Subscriber ID {} is not assigned to topic {}", id, mName);
        return false;
    }

    it->second--;
    return true;
}

// Check if low index queue items have been processed by all subscribers. Pop
// all queue elements that are no longer needed. This will be done by the slowest
// subscriber.
unsigned int Topic::clearProcessedPosts(string& subscriber_name) {
    shared_ptr<TopicQueue> q;
    if (dependencyMap.find(subscriber_name) != dependencyMap.end())
        q = mQueueMap[ dependencyMap[subscriber_name] ];
    else
        q = mQueueMap[subscriber_name];

    unique_lock<mutex> lock(q->mutex_idx);
    unsigned int minIdx = q->mMaxQueueSize;
    for (auto min_it = q->mIndexMap.begin(); min_it != q->mIndexMap.end(); min_it++) {
        if (min_it->second < minIdx)
            minIdx = min_it->second;
    }
    
    TopicQueueItem old_item;
    //unsigned int released_count = 0;
    unsigned int popped_count = 0;
    for (int i=0; i < minIdx; i++) {
        if (q->pop(old_item)) {
            //ShmManager::getInstance()->release(old_item.buffer_name);
            //released_count++;
            popped_count++;
        }
    }

    // Need to decrement subscriber indices after removing old elements
    //if (released_count > 0) {
    if (popped_count > 0) {
        for (auto dec_it = q->mIndexMap.begin(); dec_it != q->mIndexMap.end(); dec_it++)
            //dec_it->second = (dec_it->second > released_count) ? dec_it->second - released_count : 0;
            dec_it->second = (dec_it->second > popped_count) ? dec_it->second - popped_count : 0;
    }

    //return released_count;
    return popped_count;
}


// Questions:
// 1. Do we need a smaller index limit than max queue size?
// Ans: No, but we do need to consider updating the queue when the
// producer is faster than all consumers. Remove the oldest free index
// and push new data to back of queue.

// Steps for post
// 1. If queue size < max, push new data on queue
// 2. else: Find max subscriber index
// 3. if max index < queue size, replace oldest "free" queue item
// 4. If none of the above are met, do nothing.

// Steps for pull
// 1. check subscriber id and get subscriber index
// 2. while subscriber index > last queue element index, wait for other subscribers
// 3. get queue val by index and check result
// 4. pop old elements processed by all subscribers (done by slowest subscriber)
// 5. decrement subscriber indices by number of popped elements
