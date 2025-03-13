#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <string>

//#include "spdlog/spdlog.h"

using namespace std;

struct TopicQueueItem {
  string buffer_name;
  string metadata;
  uint64_t timestamp;
  TopicQueueItem(const string &name, const string &metadata, const uint64_t ts);
  TopicQueueItem() = default;
};

class TopicQueue {
private:
  deque<TopicQueueItem> mQueue;
  mutable mutex mMutex;
  condition_variable mCV;
  const unsigned int mMaxSize;
  unordered_map<string, unsigned int> mIndexMap;

  // Note: functions under private are not thread safe
  inline unsigned int size() const {return mQueue.size();}
  inline bool isUnlimited() const {return mMaxSize == 0;}
  inline bool isFull() const {return !isUnlimited() && size() >= mMaxSize;}

public:
  TopicQueue(const unsigned int maxQueueSize);
  virtual ~TopicQueue() {}

  void push_replace_oldest(TopicQueueItem &item, bool drop=true);
  bool pull(string subscriber_name, TopicQueueItem &item, int timeout = -1);
  bool decrement_index(string subscriber_name);
  unsigned int clear_old();
  void init_index(string subscriber_name);
};

class Topic {
private:
  string mName;
  bool mDropMsgs;
  mutable shared_mutex mMutex;
  condition_variable_any mCV_sub;
  unordered_map<string, shared_ptr<TopicQueue>> mQueueMap;
  unordered_map<string, string> dependencyMap;

public:
  Topic(string name, bool dropMsgs=true);
  virtual ~Topic() {}

  void post(TopicQueueItem &item);
  bool subscribe(string &subsriber_name, vector<string> &dependencies,
                 unsigned int maxQueueSize);
  bool pull(string &subsriber_name, TopicQueueItem &item, int timeout = -1);
  bool decIdx(string &subsriber_name);
  unsigned int clearProcessedPosts(string &subscriber_name);

  unsigned int size() const { return mQueueMap.size() + dependencyMap.size(); }
};
