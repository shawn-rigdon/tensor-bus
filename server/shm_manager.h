#pragma once                                                                                             
                                                                                                         
#include <string>                                                                                        
#include <unordered_map>
#include <memory>
#include <mutex>
                                                                                                         
using namespace std;                                                                                     
                                                                                                         
class ShmBuffer {                                                                                        
private:                                                                                                 
    string mName;                                                                                        
    bool mAllocated;                                                                                     
    size_t mSize;                                                                                        
    int mRefCount;                                                                                       
                                                                                                         
public:                                                                                                  
    ShmBuffer(string name);                                                                              
    virtual ~ShmBuffer();                                                                                
                                                                                                         
    bool allocate(size_t size);                                                                          
    void deallocate();                                                                                   
                                                                                                         
    inline string getName() {return mName;}                                                              
    inline int getRefCount() {return mRefCount;}                                                         
    inline void incRefCount() {mRefCount++;}                                                             
    inline void decRefCount() {mRefCount = std::max(0, mRefCount - 1);}                                  
    inline void setRefCount(int count) {mRefCount = count;}                                              
    inline size_t getSize() {return mSize;}                                                              
};                                                                                                       
                                                                                                         
                                                                                                         
class ShmManager {                                                                                       
private:                                                                                                 
    static ShmManager* instance;                                                                         
    unordered_map< string, shared_ptr<ShmBuffer> > mBuffers;                                             
    mutex mMutex;
                                                                                                         
    ShmManager() {}                                                                                      
                                                                                                         
public:                                                                                                  
    static ShmManager* getInstance() {                                                                   
        if (!instance)                                                                                   
            instance = new ShmManager();                                                                 
        return instance;                                                                                 
    }                                                                                                    
                                                                                                         
    shared_ptr<ShmBuffer> getBuffer(const string& name);                                                 
    void add(shared_ptr<ShmBuffer> shm_buf);                                                             
    void release(const string& name);                                                                    
    void releaseAll();
                                                                                                         
    ~ShmManager() {delete instance;}                                                                     
};                                                                                                       

