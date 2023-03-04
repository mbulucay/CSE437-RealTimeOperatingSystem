#ifndef MACROS_HPP__
#define MACROS_HPP__

#include <atomic>
#include <thread>


#define COMPARE_EXCHANGE_WEAK(ptr, expected, desired) \
    while(true){\
        if(ptr.compare_exchange_weak(expected, desired)){\
            break;\
        }\
    }\


#define COMPARE_EXCHANGE_STRONG(ptr, expected, desired) \
    while(true){\
        if(ptr.compare_exchange_strong(expected, desired)){\
            break;\
        }\
    }\

#define ATOMIC_FLAG_LOCK(lock) \
    int c = 0;\
    while(lock.test_and_set(std::memory_order_acquire)){\
        if(c++ >= 58){\
            std::this_thread::yield();\
        }\
    }\

#define ATOMIC_FLAG_UNLOCK(lock) \
    lock.clear(std::memory_order_release);\

#define ATOMIC_BOOL_LOCK(lock)\
    while(lock.exchange(true)){}\

#define ATOMIC_BOOL_UNLOCK(lock)\
    lock.store(false);\


#endif // ! MACROS_HPP__