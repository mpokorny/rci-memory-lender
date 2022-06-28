#include <vector>
#include <queue>
#include <memory>
#include <new>
#include <set>
#include <mutex>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <condition_variable>

#include "lender.hpp"

template <typename T>
library<T>::library(const std::vector<size_t> s, unsigned int max) : 
    recycle_memory<T>::recycle_memory(s, max) 
{
    change_q = std::deque<buffer_ptr<T>>();
}

template <typename T>
auto library<T>::fill() -> buffer_ptr<T> {

    std::unique_lock<std::mutex> lock(this->free_mutex);
    while(this->free_q.empty()) {  
        this->free_variable.wait(lock);
    }

    // take from free list
    T* ptr = this->free_q.front();
    this->free_q.pop_front();

    #ifndef NDEBUG
    // check there was no changes after free
    T* f = new T();
    memset(f, 0xf0, sizeof(T));
    assert(memcmp(ptr, f, sizeof(T)) == 0);
    memset(ptr, 0, sizeof(T)*this->size);
    #endif

    // make reuseable_buffer for the buffer
    auto sp = buffer_ptr<T>(ptr, *this);
    return sp;

}

template <typename T>
void library<T>::queue(buffer_ptr<T> ptr) {
    std::unique_lock<std::mutex> guard(change_mutex);
    #ifndef NDEBUG
    assert(this->pointers.find(ptr.get()) != this->pointers.end());
    #endif
    change_q.push_back(ptr);
    guard.unlock();
    change_variable.notify_one();
}

template <typename T>
auto library<T>::operate() -> buffer_ptr<T> {

    std::unique_lock<std::mutex> lock(change_mutex);
    while (change_q.empty()) {
        change_variable.wait(lock);
    }

    buffer_ptr<T> r = change_q.front();
    change_q.pop_front();
    change_mutex.unlock();

    return r;
}

template <typename T>
auto library<T>::change_condition() -> bool {
    return !change_q.empty();
}


template <typename T>
auto library<T>::private_queue_size() -> int {
    return change_q.size();
}