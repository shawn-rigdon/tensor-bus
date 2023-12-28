#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <vector>

//#include "spdlog/spdlog.h"

using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std;

template <typename Data> class ThreadSafeQueue {
protected:
  deque<Data> mQueue;
  mutable mutex mMutex;
  condition_variable mCV;

public:
  // TODO: should I use notify_one or notify_all?
  void push(Data const &data) {
    unique_lock<mutex> lock(mMutex);
    mQueue.push_back(data);
    lock.unlock();
    mCV.notify_one();
  }

  void push_front(Data const &data) {
    unique_lock<mutex> lock(mMutex);
    mQueue.push_front(data);
    lock.unlock();
    mCV.notify_one();
  }

  unsigned int size() const {
    unique_lock<mutex> lock(mMutex);
    return mQueue.size();
  }

  bool empty() const {
    unique_lock<mutex> lock(mMutex);
    return mQueue.empty();
  }

  bool pop(Data &val) {
    unique_lock<mutex> lock(mMutex);
    if (mQueue.empty())
      return false;

    val = mQueue.front();
    mQueue.pop_front();
    return true;
  }

  bool wait_and_pop(Data &val, int timeout_ms = -1) {
    auto now = system_clock::now();
    unique_lock<mutex> lock(mMutex);
    while (mQueue.empty()) {
      if (timeout_ms != -1) {
        if (std::cv_status::timeout ==
            mCV.wait_until(lock, now + timeout_ms * 1ms))
          return false;
      } else
        mCV.wait(lock);
    }

    val = mQueue.front();
    mQueue.pop_front();
    return true;
  }

  bool get_val_by_index(Data &val, unsigned int idx) {
    unique_lock<mutex> lock(mMutex);
    if (mQueue.size() <= idx)
      return false;

    val = mQueue[idx];
    return true;
  }

  void erase(unsigned int idx) { mQueue.erase(mQueue.begin() + idx); }

  //    bool wait_get_val_by_index(Data& val, int idx) {
  //        auto now = system_clock::now();
  //        unique_lock<mutex> lock(mMutex);
  //        while (mQueue.size() <= idx) {
  //            if (timeout_ms != -1) {
  //                if (std::cv_status::timeout == mCV.wait_until(lock, now +
  //                timeout_ms*1ms))
  //                    return false;
  //            } else
  //                mCV.wait(lock);
  //        }
  //
  //        //TODO: do I need this function?
  //        return true;
  //    }
};

class TopicQueueItem {
public:
  string buffer_name;
  string metadata;
  uint64_t timestamp;
  TopicQueueItem(const string &name, const string &metadata, const uint64_t ts);
  TopicQueueItem() = default;
};

class TopicQueue : public ThreadSafeQueue<TopicQueueItem> {
public:
  unsigned int mMaxQueueSize;
  mutex mutex_idx;
  condition_variable cv_idx, cv_post_block;
  unordered_map<string, unsigned int> mIndexMap;

  TopicQueue(const unsigned int maxQueueSize);
  virtual ~TopicQueue() {}

  void get(TopicQueueItem &item);
  void put(const TopicQueueItem &item);
};

class Topic {
private:
  string mName;
  bool mDropMsgs;
  mutex mMutex;
  condition_variable mCV;
  unordered_map<string, shared_ptr<TopicQueue>> mQueueMap;
  unordered_map<string, string> dependencyMap;

public:
  Topic(string name, bool dropMsgs=true);
  virtual ~Topic() {}

  void post(TopicQueueItem &item);
  bool subscribe(string &subsriber_name, std::vector<string> &dependencies,
                 unsigned int maxQueueSize);
  bool pull(string &subsriber_name, TopicQueueItem &item, int timeout = -1);
  bool decIdx(string &subsriber_name);
  unsigned int clearProcessedPosts(string &subscriber_name);

  unsigned int size() const { return mQueueMap.size() + dependencyMap.size(); }
};
