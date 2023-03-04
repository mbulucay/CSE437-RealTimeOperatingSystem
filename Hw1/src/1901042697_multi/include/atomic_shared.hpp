#ifndef ATOMIC_SHARED_HPP__
#define ATOMIC_SHARED_HPP__

#include <atomic> 
#include <memory> 
#include <iostream>

template <class T>
class AtomicSharedPtr
{
private: 
    
    std::atomic<std::shared_ptr<T>> ptr;

public:

    AtomicSharedPtr() : ptr(nullptr) {}

    AtomicSharedPtr(std::shared_ptr<T> p){
        ptr.store(p);
    }

    std::shared_ptr<T> load() const{
        return ptr.load();
    }

    void store(std::shared_ptr<T> p)
    {
        ptr.store(p);
    }

    std::shared_ptr<T> exchange(std::shared_ptr<T> p)
    {   
        while(ptr.exchange(p) != p)
        { }
        return p;
    }

    bool compare_exchange_weak(std::shared_ptr<T>& expected, std::shared_ptr<T> desired)
    {
        while(ptr.compare_exchange_weak(expected, desired) != desired)
        { }
        return desired;
    }

    bool compare_exchange_strong(std::shared_ptr<T>& expected, std::shared_ptr<T> desired)
    {
        while(ptr.compare_exchange_strong(expected, desired) != desired)
        { }
        return desired;
    }
};

#endif // !ATOMIC_SHARED_HPP__
