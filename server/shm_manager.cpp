#include "shm_manager.h"

#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

ShmManager* ShmManager::instance = nullptr;

ShmBuffer::ShmBuffer(string name) :
    mName(name),
    mAllocated(false),
    mSize(0),
    mRefCount(0)
{
}

ShmBuffer::~ShmBuffer() {
    if (mAllocated)
        deallocate();
}

bool ShmBuffer::allocate(size_t size) {
    if (mAllocated) {
        cout << "shm buffer " << mName << " is already allocated" << endl;
        return false;
    }

    int fd = shm_open(mName.c_str(), O_CREAT | O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    if (fd >= 0) {
        if (ftruncate(fd, size) >= 0) {
            mAllocated = true;
            mSize = size;
        } else
            cout << "Failed to allocate shm buffer " << mName << endl;

        close(fd);
    }
    return mAllocated;
}

void ShmBuffer::deallocate() {
    if (mAllocated) {
        shm_unlink(mName.c_str());
        mAllocated = false;
    }
}

shared_ptr<ShmBuffer> ShmManager::getBuffer(const string& name) {
    lock_guard<mutex> lock(mMutex);
    auto it = mBuffers.find(name);
    return it == mBuffers.end() ? shared_ptr<ShmBuffer>() : it->second;
}

void ShmManager::add(shared_ptr<ShmBuffer> shm_buf) {
    lock_guard<mutex> lock(mMutex);
    mBuffers[shm_buf->getName()] = shm_buf;
}

void ShmManager::release(const string& name) {
    lock_guard<mutex> lock(mMutex);
    auto it = mBuffers.find(name);
    if (it != mBuffers.end()) {
        it->second->decRefCount();
        if (it->second->getRefCount() == 0)
            mBuffers.erase(name);
    }
}

void ShmManager::releaseComplete(const string& name) {
    lock_guard<mutex> lock(mMutex);
    auto it = mBuffers.find(name);
    if (it != mBuffers.end()) {
        it->second->setRefCount(0);
        mBuffers.erase(name);
    }
}

void ShmManager::releaseAll() {
    lock_guard<mutex> lock(mMutex);
    for (auto& it: mBuffers)
        it.second->setRefCount(0);

    mBuffers.clear();
}
