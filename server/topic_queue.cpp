#include "topic_queue.h"
#include "shm_manager.h"

TopicQueueItem::TopicQueueItem(const string& name, const uint64_t ts) :
    buffer_name(name),
    timestamp(ts)
{
}

Topic::Topic(string name, unsigned int maxQueueSize) :
    mName(name),
    mMaxQueueSize(maxQueueSize)
{
}

// If the subscriber is not already subscribed to the topic
// the given subscriber name is set to the oldest position in the queue.
// This subscriber method allows multiple subscribers of the same name.
bool Topic::subscribe(string& subscriber_name) {
    unique_lock<mutex> lock(mMutex);
    auto it = mIndexMap.find(subscriber_name);
    if (it == mIndexMap.end())
        mIndexMap[subscriber_name] = 0;

    return true;
}

void Topic::post(TopicQueueItem& item) {
    if (mMaxQueueSize == 0 || mQueue.size() < mMaxQueueSize) {
        mQueue.push(item);
        mCV.notify_all();
        return;
    }

    // Remove the oldest queue element not currently being processed by
    // a subscriber. This keeps the topic up to date.
    // Oldest free topic is at the max subscriber index + 1

    unsigned int maxIdx = 0;
    unique_lock<mutex> lock(mMutex);
    for (auto it = mIndexMap.begin(); it != mIndexMap.end(); it++) {
        if (it->second > maxIdx)
            maxIdx = it->second;
    }

    if (maxIdx < mMaxQueueSize) { // mMaxQueueSize = mQueue.size() if this block is executed
        int removeIdx = maxIdx + 1;
        TopicQueueItem removeItem;
        mQueue.get_val_by_index(removeItem, removeIdx);
        ShmManager::getInstance()->release(removeItem.buffer_name);
        mQueue.erase(removeIdx);
        mQueue.push(item);
    }
}

// this should set item according to the subscriber id's index, increment the index,
// and pop any elements that have been seen by all subscribers. If the current index
// is greater than the number of queue elements, block until data is available.
bool Topic::pull(string& subscriber_name, TopicQueueItem& item) {
    unique_lock<mutex> lock(mMutex);
    auto it = mIndexMap.find(subscriber_name);
    if (it == mIndexMap.end()) {
        //spdlog::error("Subscriber ID {} is not assigned to topic {}", id, mName);
        return false;
    }

    // If the current subscriber has processed all available queue messages,
    // it should wait for other subscribers to free up old messages and/or
    // the publisher to post new data
    while (it->second >= mQueue.size()) { // it->second retrieves current subscriber index
        //spdlog::debug("Subscriber {} is waiting for new data", id);
        mCV.wait(lock);
    }

    mQueue.get_val_by_index(item, it->second++); // note the index is incremented
    return true;
}

bool Topic::decIdx(string& subscriber_name) {
    unique_lock<mutex> lock(mMutex);
    auto it = mIndexMap.find(subscriber_name);
    if (it == mIndexMap.end()) {
        //spdlog::error("Subscriber ID {} is not assigned to topic {}", id, mName);
        return false;
    }

    it->second--;
    return true;
}

// Check if low index queue items have been processed by all subscribers. Pop
// all queue elements that are no longer needed. This will be done by the slowest
// subscriber.
unsigned int Topic::clearProcessedPosts() {
    unique_lock<mutex> lock(mMutex);
    unsigned int minIdx = mMaxQueueSize;
    for (auto min_it = mIndexMap.begin(); min_it != mIndexMap.end(); min_it++) {
        if (min_it->second < minIdx)
            minIdx = min_it->second;
    }
    
    TopicQueueItem old_item;
    //unsigned int released_count = 0;
    unsigned int popped_count = 0;
    for (int i=0; i < minIdx; i++) {
        if (mQueue.pop(old_item)) {
            //ShmManager::getInstance()->release(old_item.buffer_name);
            //released_count++;
            popped_count++;
        }
    }

    // Need to decrement subscriber indices after removing old elements
    //if (released_count > 0) {
    if (popped_count > 0) {
        for (auto dec_it = mIndexMap.begin(); dec_it != mIndexMap.end(); dec_it++)
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
