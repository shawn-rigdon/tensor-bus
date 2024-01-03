#include "topic_queue.h"
#include "shm_manager.h"
#include "spdlog/spdlog.h"
#include <iostream>

using namespace std::chrono;
using namespace std::chrono_literals;

TopicQueueItem::TopicQueueItem(const string &name, const string &metadata,
                               const uint64_t ts)
    : buffer_name(name), metadata(metadata), timestamp(ts) {}

TopicQueue::TopicQueue(unsigned int maxQueueSize)
    : mMaxSize(maxQueueSize) {}

// If the queue is full, replace the oldest data not processed by a subscriber
void TopicQueue::push_replace_oldest(TopicQueueItem &item, bool drop) {
    // If dropping msgs, block for each queue if it is full. This will
    // throttle the topic to the speed of the slowest subscriber, but
    // is necessary to avoid consuming all system memory. Therefore the
    // developer should be sure blocking is necessary.
    unique_lock lock(mMutex);
    while (!drop && isFull())
      mCV.wait(lock);

    if (!isFull()) {
      mQueue.push_back(item);
      mCV.notify_all();
      return;
    }

    // Remove the oldest queue element not currently being processed by
    // a subscriber. This keeps the topic up to date.
    // Oldest free topic is at the max subscriber index + 1
    unsigned int maxIdx = 0;
    for (auto it = mIndexMap.begin(); it != mIndexMap.end(); it++) {
      if (it->second > maxIdx)
        maxIdx = it->second;
    }

    unsigned int removeIdx = maxIdx + 1;
    if (removeIdx >= size()) {
      ShmManager::getInstance()->release(item.buffer_name, mIndexMap.size());
      return;
    }

    TopicQueueItem removeItem = mQueue[removeIdx];
    mQueue.erase(mQueue.begin() + removeIdx);
    mQueue.push_back(item);
    mCV.notify_all();
    ShmManager::getInstance()->release(removeItem.buffer_name, mIndexMap.size());
}

bool TopicQueue::pull(string subscriber_name, TopicQueueItem &item, int timeout) {
  unique_lock lock(mMutex);
  auto it = mIndexMap.find(subscriber_name);
  if (it == mIndexMap.end()) {
    // spdlog::error("Subscriber ID {} is not assigned to topic {}", id, mName);
    return false;
  }
  int idx = it->second++; // Note: idx is incremented

  // If the current subscriber has processed all available queue messages,
  // it should wait for other subscribers to free up old messages and/or
  // the publisher to post new data
  while (idx >= size()) { // it->second retrieves current subscriber index
    // spdlog::debug("Subscriber {} is waiting for new data", id);
    if (timeout < 0) {
        mCV.wait(lock);
    }
    else {
        mCV.wait(lock);
      auto status = mCV.wait_until(
              lock, system_clock::now() + timeout * 1ms);
        mCV.wait(lock);
      if (status == std::cv_status::timeout)
        return false; // return false if timeout
    }
  }

  item = mQueue[idx];
  return true;
}

bool TopicQueue::decrement_index(string subscriber_name) {
  lock_guard lock(mMutex);
  auto it = mIndexMap.find(subscriber_name);
  if (it == mIndexMap.end()) {
    return false;
  }

  it->second--;
  return true;
}

unsigned int TopicQueue::clear_old() {
  lock_guard lock(mMutex);
  unsigned int minIdx = mMaxSize;
  for (auto it = mIndexMap.begin(); it != mIndexMap.end(); it++) {
    if (it->second < minIdx)
      minIdx = it->second;
  }

  unsigned int popped_count = 0;
  for (int i = 0; i < minIdx; ++i) {
    if (mQueue.empty())
      break;

    mQueue.pop_front();
    popped_count++;
    mCV.notify_one();
  }

  // Need to decrement subscriber indices after removing old elements
  if (popped_count > 0) {
    for (auto it = mIndexMap.begin(); it != mIndexMap.end(); it++)
      it->second = (it->second > popped_count) ? it->second - popped_count : 0;
  }

  return popped_count;
}

void TopicQueue::init_index(string subscriber_name) {
  lock_guard lock(mMutex);
  mIndexMap[subscriber_name] = 0;
}

Topic::Topic(string name, bool dropMsgs) : mName(name), mDropMsgs(dropMsgs) {}

// If the subscriber is not already subscribed to the topic
// the given subscriber name is set to the oldest position in the queue.
// This subscriber method allows multiple subscribers of the same name.
bool Topic::subscribe(string &subscriber_name,
                      std::vector<string> &dependencies,
                      unsigned int maxQueueSize) {
  unique_lock<shared_mutex> lock(mMutex);
  if (dependencies.size() > 0) {
    if (dependencyMap.find(subscriber_name) != dependencyMap.end())
      return true;

    string foundName;
    bool foundDependency = false;
    while (true) {
      // TODO: this only handles 1 dependency correctly
      for (int i = 0; i < dependencies.size(); ++i) {
        if (mQueueMap.find(dependencies[i]) != mQueueMap.end()) {
          foundDependency = true;
          foundName = dependencies[i];
          break;
        }
      }

      if (foundDependency)
        break;

      mCV_sub.wait(lock);
    }
    dependencyMap[subscriber_name] = foundName; // point to a parent queue
    mQueueMap[foundName]->init_index(subscriber_name);
  } else if (mQueueMap.find(subscriber_name) == mQueueMap.end()) {
    mQueueMap[subscriber_name] = make_shared<TopicQueue>(maxQueueSize);
    mQueueMap[subscriber_name]->init_index(subscriber_name);
    mCV_sub.notify_all();
  }

  return true;
}

void Topic::post(TopicQueueItem &item) {
  shared_lock lock(mMutex); // need read access to mQueueMap
  for (auto q_it = mQueueMap.begin(); q_it != mQueueMap.end(); ++q_it) {
    shared_ptr<TopicQueue> q = q_it->second;
    q->push_replace_oldest(item, mDropMsgs);
  }
}

// this should set item according to the subscriber id's index, increment the
// index, and pop any elements that have been seen by all subscribers. If the
// current index is greater than the number of queue elements, block until data
// is available by default. If block false immediately return when there is no
// available data
bool Topic::pull(string &subscriber_name, TopicQueueItem &item, int timeout) {
  shared_lock lock(mMutex); // need read access to mQueueMap and dependencyMap
  shared_ptr<TopicQueue> q;
  if (dependencyMap.find(subscriber_name) != dependencyMap.end())
    q = mQueueMap[dependencyMap[subscriber_name]];
  else if (mQueueMap.find(subscriber_name) != mQueueMap.end())
    q = mQueueMap[subscriber_name];
  else
    return false;

  return q->pull(subscriber_name, item, timeout);
}

bool Topic::decIdx(string &subscriber_name) {
  shared_lock lock(mMutex);
  shared_ptr<TopicQueue> q;
  if (dependencyMap.find(subscriber_name) != dependencyMap.end())
    q = mQueueMap[dependencyMap[subscriber_name]];
  else
    q = mQueueMap[subscriber_name];

  return q->decrement_index(subscriber_name);
}

// Check if low index queue items have been processed by all subscribers. Pop
// all queue elements that are no longer needed. This will be done by the
// slowest subscriber.
unsigned int Topic::clearProcessedPosts(string &subscriber_name) {
  shared_lock lock(mMutex);
  shared_ptr<TopicQueue> q;
  if (dependencyMap.find(subscriber_name) != dependencyMap.end())
    q = mQueueMap[dependencyMap[subscriber_name]];
  else if (mQueueMap.find(subscriber_name) != mQueueMap.end())
    q = mQueueMap[subscriber_name];
  else
    return 0;

  return q->clear_old();
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
// 2. while subscriber index > last queue element index, wait for other
// subscribers
// 3. get queue val by index and check result
// 4. pop old elements processed by all subscribers (done by slowest subscriber)
// 5. decrement subscriber indices by number of popped elements
