#ifndef THREAD_SAFE_SET_HPP__
#define THREAD_SAFE_SET_HPP__

#include <memory>
#include <atomic>
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
    ~ThreadSafeSet(){
        clear();
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


    bool insert(const T& value){

        ATOMIC_FLAG_LOCK(flag);
        std::atomic<std::shared_ptr<Node>> local;
        local.store(root.load());
        bool added = false;
        while (true)
        {
            std::shared_ptr<Node> v = local.load();
            std::shared_ptr<Node> result;
            COMPARE_EXCHANGE_WEAK(local, v, result);
            result = add(v, value, added);

            if(result != nullptr){
                root.store(result);
                break;
            }
        }
        ATOMIC_FLAG_UNLOCK(flag);

        return added;
    }

    bool remove(const T& value){ 

        ATOMIC_FLAG_LOCK(flag);
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
        ATOMIC_FLAG_UNLOCK(flag);

        return removed;
    }

    bool search(const T& value) const {     

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

    void clear() {
        ATOMIC_FLAG_LOCK(flag);
        root.store(nullptr);
        ATOMIC_FLAG_UNLOCK(flag);
    }

    void iterate(const std::function<void(const T&)>& func) {
        std::atomic<std::shared_ptr<Node>> local;
        local.store(root.load());
        iterate(local.load(), func);
    }


private:


    /* 
        We can improve the performance using parent & grandparent locks but we need to resolve the returning values for parent storage 
        
        Comment codes are for parent & grandparent locks but load & store operations are returning value for a node and a node cant handle that
        situation. 
        
        We need to find a way to store the parent & grandparent nodes and return them to the add & remove functions.
        This way we can improve the performance. nearly %75 percentage 
    */
    
    // std::shared_ptr<Node> add(std::shared_ptr<Node> _local, T value, std::shared_ptr<Node> parent, std::shared_ptr<Node> gParent ,bool& added){
    std::shared_ptr<Node> add(std::shared_ptr<Node> _local, T value, bool& added){

        std::atomic<std::shared_ptr<Node>> local;
        local.store(_local);

        // if(gParent != nullptr){
        //     ATOMIC_FLAG_UNLOCK(gParent->marked);
        // }  
        // if(local.load() != nullptr){
        //     ATOMIC_FLAG_LOCK(local.load()->marked);
        // }

        if(local.load() == nullptr){
            added = true;
            return std::make_shared<Node>(value);
        }else{
            if(value < local.load()->value){

                std::shared_ptr<Node> v = local.load()->left;
                COMPARE_EXCHANGE_WEAK(local.load()->left, v, add(v, value, added));
                // local.load()->left = add(local.load()->left, value, local.load(), added);
            }
            
            if(!(value < local.load()->value) && !(value == local.load()->value)){
                std::shared_ptr<Node> v = local.load()->right;
                COMPARE_EXCHANGE_WEAK(local.load()->right, v, add(v, value, added));
                // local.load()->right = add(local.load()->right, value, local.load(), added);
            }
        }

        // ATOMIC_FLAG_UNLOCK(local.load()->marked);
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


    T findMin(std::shared_ptr<Node> local){
        while(local->left.load() != nullptr){
            local = local->left;
        }
        return local->value;
    }


    T findMax(std::shared_ptr<Node> local){
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


    void iterate(std::shared_ptr<Node> local, std::function<void(T)> f) const {
        if(local != nullptr){
            iterate(local->left, f);
            f(local->value);
            iterate(local->right, f);
        }
    }


    struct Node
    {
        T value;
        std::atomic<std::shared_ptr<Node>> left;
        std::atomic<std::shared_ptr<Node>> right;

        std::atomic_flag marked = ATOMIC_FLAG_INIT;


        Node(T value) : value(value), left(nullptr), right(nullptr) {}

        void wait_lock(){
            ATOMIC_FLAG_LOCK(marked);
        }

        void unlock(){
            ATOMIC_FLAG_UNLOCK(marked);
        }

    };

    std::atomic<std::shared_ptr<Node>> root;
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

};

} // namespace mbu

#endif // !THREAD_SAFE_SET_H__