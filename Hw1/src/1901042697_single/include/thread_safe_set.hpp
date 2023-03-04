#ifndef THREAD_SAFE_SET_HPP__
#define THREAD_SAFE_SET_HPP__

#include <memory>
#include <atomic>
#include <string>
#include <type_traits>
#include <concepts>
#include <functional>

#include "macros.hpp"
#include "requirements.hpp"

namespace mbu{

template <class T>
class ThreadSafeSet
{
struct Node;

public:

    ThreadSafeSet(){
        static_assert(has_less_than<T>, "T must have operator<");
        static_assert(has_equal_to<T>, "T must have operator==");
    }

    ThreadSafeSet(const ThreadSafeSet& other) = delete;
    ThreadSafeSet& operator=(const ThreadSafeSet& other) = delete;
    
    ThreadSafeSet(ThreadSafeSet&& other){
        root.store(other.root.load());
        other.root.store(nullptr);
    };
    
    ThreadSafeSet& operator=(ThreadSafeSet&& other){
        root.store(other.root.load());
        other.root.store(nullptr);
        return *this;
    };

    ~ThreadSafeSet(){
        clear();
    }

    bool insert(const T& value){

        std::atomic<std::shared_ptr<Node>> local;
        local.store(root.load());

        bool added = false;
        while (true)
        {
            std::shared_ptr<Node> v = local.load();
            std::shared_ptr<Node> result;

            COMPARE_EXCHANGE_WEAK(local, v, result);
            result = insert(v, value, added);

            if(result != nullptr){
                root.store(result);
                break;
            }
        }

        return added;
    }

    bool remove(const T& value){ 

        std::atomic<std::shared_ptr<Node>> local;
        local.store(root.load());
        bool removed = false;
        while (true)
        {
            std::shared_ptr<Node> v = local.load();
            std::shared_ptr<Node> result;

            COMPARE_EXCHANGE_WEAK(local, v, result);
            result = remove(v, value, removed);

            if(result != nullptr){
                root.store(result);
                break;
            }
        }

        return removed;
    }

    bool search(const T& value) const{     

        std::atomic<std::shared_ptr<Node>> local;
        local.store(root.load());
        bool result = search(local.load(), value);

        return result;
    }

    int size() const {
        std::atomic<std::shared_ptr<Node>> local;
        local.store(root.load());
        return size(local.load());
    }

    bool empty() const {
        std::atomic<std::shared_ptr<Node>> local;
        local.store(root.load());

        return local.load() == nullptr;
    }

    void clear(){
        ATOMIC_FLAG_LOCK(lock);
        root.store(nullptr);
        ATOMIC_FLAG_UNLOCK(lock);
    }

    void iterate(const std::function<void(const T& obj)>& func) {
        std::atomic<std::shared_ptr<Node>> local;
        local.store(root.load());
        iterate(local.load(), func);
    }

private:

    std::shared_ptr<Node> insert(std::shared_ptr<Node> _local, T value, bool& added){

        std::atomic<std::shared_ptr<Node>> local;
        local.store(_local);

        if(local.load() == nullptr){
            added = true;
            return std::make_shared<Node>(value);
        }else{

            if(value < local.load()->value){

                std::shared_ptr<Node> v = local.load()->left;
                COMPARE_EXCHANGE_WEAK(local.load()->left, v, insert(v, value, added));
            }
            if(!(value < local.load()->value) && !(value == local.load()->value)){
                std::shared_ptr<Node> v = local.load()->right;
                COMPARE_EXCHANGE_WEAK(local.load()->right, v, insert(v, value, added));
            }
        }

        return local.load();
    }

    std::shared_ptr<Node> remove(std::shared_ptr<Node> _local, T value, bool& removed){

        std::atomic<std::shared_ptr<Node>> local;
        local.store(_local);

        if(local.load() == nullptr){
            return local;
        }
        if(value < local.load()->value){
            
            std::shared_ptr<Node> v = local.load()->left;
            COMPARE_EXCHANGE_WEAK(local.load()->left, v, remove(v, value, removed));

        }else if(!(value < local.load()->value) && !(value == local.load()->value)){

            std::shared_ptr<Node> v = local.load()->right;
            COMPARE_EXCHANGE_WEAK(local.load()->right, v, remove(v, value, removed));

        }else{

            removed = true;
            if(local.load()->left.load() == nullptr && local.load()->right.load() == nullptr){

                std::shared_ptr<Node> v = local;
                COMPARE_EXCHANGE_WEAK(local, v, nullptr);

            }else if(local.load()->left.load() == nullptr){

                std::shared_ptr<Node> v = local;
                COMPARE_EXCHANGE_WEAK(local, v, local.load()->right);

            }else if(local.load()->right.load() == nullptr){
                std::shared_ptr<Node> v = local;
                COMPARE_EXCHANGE_WEAK(local, v, local.load()->left);

            }else{
                
                T max = findMax(local.load()->left);
                local.load()->value = max;

                std::shared_ptr<Node> v = local.load()->left;
                COMPARE_EXCHANGE_WEAK(local.load()->left, v, remove(v, max, removed));
                local.load()->right.store(remove(local.load()->right, local.load()->value, removed));
            }
        }

        return local;
    }


    T findMin(std::shared_ptr<Node> local) const{
        while(local->left.load() != nullptr){
            local = local->left;
        }
        return local->value;
    }

    
    T findMax(std::shared_ptr<Node> local) const {
        while(local->right.load() != nullptr){
            local = local->right;
        }
        return local->value;
    }


    bool search(std::shared_ptr<Node> _local, T value) const {

        std::atomic<std::shared_ptr<Node>> local;
        local.store(_local);

        if(local.load() == nullptr){
            return false;
        }
        if(value < local.load()->value){
            return search(local.load()->left, value);
        }else if(!(value < local.load()->value) && !(value == local.load()->value)){
            return search(local.load()->right, value);
        }else{
            return true;
        }
    }


    int size(std::shared_ptr<Node> local) const {
        if(local == nullptr)
            return 0;
        else
            return 1 + size(local->left) + size(local->right);
    }


    void iterate(std::shared_ptr<Node> local, const std::function<void(const T& obj)>& func) const {
        if(local != nullptr){
            iterate(local->left, func);
            func(local->value);
            iterate(local->right, func);
        }
    }
  

    struct Node
    {
        T value;
        std::atomic<std::shared_ptr<Node>> left;
        std::atomic<std::shared_ptr<Node>> right;

        Node(const T& value) : value(value), left(nullptr), right(nullptr) {}
    };

    std::atomic<std::shared_ptr<Node>> root;
    std::atomic_flag lock = ATOMIC_FLAG_INIT;

};

} // namespace mbu

#endif // !THREAD_SAFE_SET_HPP__