#ifndef R3_H
#define R3_H

#include <vector>
#include <queue>
#include <complex>
#include <atomic>
#include <memory>
#include <new>
#include <unistd.h>

#include <iostream>

template <typename T>
class recycle_memory;

// The base class for the pointer will need to be extendable to ALL types that
// the memory system will use
// NOTE: this is a very shallow class, as the memory management is going to be
// dealing with handing the pointer to threads


template <typename T>
struct reuseable_buffer {
    // TODO: array indexing operations

    // TODO: check whether this kills the memory in the shared pointer while we are at it
    T* ptr; // this could be a shared_pointewr
    std::vector<size_t> shape;

    reuseable_buffer(std::vector<size_t> s, T* p, recycle_memory<T>& r) : recycle(r) {
        shape = s;
        ptr = p;
    }

    // TODO: is it needed to explicitly call the shared_ptr destructor?
    ~reuseable_buffer() {
        recycle._return_memory(ptr);
    };

    private:
        recycle_memory<T>& recycle;
};

/* buffer typedef for shared_ptrs */
template <typename T>
using buffer_ptr = std::shared_ptr<reuseable_buffer<T>>;

/* recycle_memory class will both MAKE and DESTROY memory that is within the reuseable_buffer class
 *
 * recycle_memory needs to store unused memory for a particular type
 * needs to store queues for threads to take from
 */
template <typename T>
class recycle_memory {
    // has a vector of recycle_memory struct pointers.
    private:
        std::vector<size_t> shape;
        std::queue<buffer_ptr<T>> change;
        std::queue<T*> free;

    public:
        /*
         * Instantiator takes in list of all type signatures with their shapes
         * (and maybe names - might require names actually)
         *
         * for each type, expect:
         *     type
         *     shape
         *     maximum number (if applicable)
         *     constructor
         *     destructor
         */
        recycle_memory(std::vector<size_t> s, unsigned int max) {
            change = std::queue<buffer_ptr<T>>();
            free = std::queue<T*>();
            shape = s;

            // make all the new ints here to put in the free list
            for (unsigned int i = 0; i < max; i++) {
                // use nothrow because we don't do anything with the exception
                T* temp = new(std::nothrow) T();
                // TODO: for testing this should be set to a known value
                if (temp == nullptr) {
                    // we could set a different value as max in the object
                    break;
                }
                free.push(temp);
            }
        }
        /*
         * Destructor must destroy ALL memory that was allocated using their
         * corresponding destructors
         */
        ~recycle_memory() {
            // TODO: make sure all buffers have released their memory to free

            while (!free.empty()) {
                T* temp = free.front(); 
                free.pop();
                if (temp != NULL) {
                    delete temp;
                }
            }
        }

        /* get a shared pointer to the buffer we want to fill with data */
        buffer_ptr<T> fill() {

            // if free list is empty, wait for memory to be freed
            while (free.empty()) {
                sleep(0.1);
            }
            // else take from free list
            T* ptr = free.front();
            free.pop();

            // make reuseable_buffer for the buffer
            auto sp = std::make_shared<reuseable_buffer<T>>(shape, ptr, *this);
            return sp;

        }

        /* give a shared pointer back to be queued for operation */
        void queue(buffer_ptr<T> ptr) {
            change.push(ptr);
            return;
        }

        /* get a shared pointer from the queue to operate on */
        buffer_ptr<T> operate() {
            // take a reuseable_buffer off the queue - block until we have one
            // TODO: need to make these queues locking for safety
            while (change.empty()) {
                sleep(0.1);
            }
            buffer_ptr<T> r = change.front();
            change.pop();

            return r;
        }

        void _return_memory(T* p) {
            free.push(p);
            return;
        }

        int _free_size() {
            return free.size();
        }

        int _queue_size() {
            return change.size();
        }
};

// TODO: make memory_collection: variadic template for as many buffer types as we want.

#endif
