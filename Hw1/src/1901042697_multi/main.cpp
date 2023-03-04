#include <iostream>
#include <thread>
#include <random>
#include <iomanip>
#include <algorithm>
#include <chrono>

#include "./include/thread_safe_set.hpp"
#include "./include/custom_type.hpp"
#include "./include/random_generator.hpp"


int main(){
    
    constexpr int SIZE = 100000;
    std::vector<int> values(2*SIZE+1);
    std::iota(begin(values), end(values), 0);
    std::random_shuffle(begin(values), end(values));
    values.resize(SIZE);

    int num_threads = 10;
    int chunk_size = SIZE / num_threads;

    mbu::ThreadSafeSet<CustomType> set;
    std::vector<std::thread> insert_threads;
    std::vector<std::thread> remove_threads;
    std::vector<std::thread> contains_threads;

    int added, removed, contains;
    added = removed = contains = 0;

    auto insertFoo_Lvalue = [&](int s, int e)-> void {
        for (int i = s; i < e; ++i) {
            CustomType value(values[i]);
            if(set.insert(value))
                ++added;
        }
    };


    auto insertFoo_Move = [&](int s, int e)-> void {
        for (int i = s; i < e; ++i) {
            CustomType value(values[i]);
            if(set.insert(std::move(value)))
                ++added;
        }
    };

    auto insertFoo_Copy = [&](int s, int e)-> void {
        for (int i = s; i < e; ++i) {
            if(set.insert(CustomType(values[i])))
                ++added;
        }
    };

    auto removeFoo = [&](int s, int e)-> void {
        for (int i = s; i < e; ++i) {
            if(set.remove(CustomType(values[i])))
                ++removed;
        }
    };

    auto containsFoo = [&](int s, int e)-> void {
        for (int i = s; i < e; ++i) {
            if(set.search(CustomType(values[i])))
                ++contains;
        }
    };

    auto start = std::chrono::high_resolution_clock::now();

    for(int i = 0; i < num_threads/3; ++i){
        insert_threads.push_back(std::thread(insertFoo_Lvalue, i*chunk_size, (i+1)*chunk_size));
    }

    for(int i = num_threads/3; i < 2*num_threads/3; ++i){
        insert_threads.push_back(std::thread(insertFoo_Move, i*chunk_size, (i+1)*chunk_size));
    }

    for(int i = 2*num_threads/3; i < num_threads; ++i){
        insert_threads.push_back(std::thread(insertFoo_Copy, i*chunk_size, (i+1)*chunk_size));
    }

    for(int i = 0; i < num_threads; ++i){
        if(i % 2 == 0)
            remove_threads.push_back(std::thread(removeFoo, i*chunk_size, (i+1)*chunk_size));
    }

    /* 
        if you want to increase the chance of exist in the numbers in tree increase the number
        but at some point half of the numbers will be removed and the max number will be half of the size
        1 - 300 millisecond you could try for size 100000
    */
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // std::this_thread::sleep_for(std::chrono::nanoseconds(20));


    for(int i = 0; i < num_threads; ++i){
        if(i % 2 == 1)
            contains_threads.push_back(std::thread(containsFoo, i*chunk_size, (i+1)*chunk_size));
    }

    for(auto& thread : insert_threads)
        thread.join();

    for(auto& thread : remove_threads)
        thread.join();
    
    for(auto& thread : contains_threads)
        thread.join();

    auto end = std::chrono::high_resolution_clock::now();

    set.iterate([](const CustomType& obj){
        std::cout << obj << ", ";
    });

    std::cout << std::endl;
    std::cout << std::endl;


    std::cout << "Insertion time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
    std::cout << "Size: " << set.size() << std::endl;
    std::cout << "Added: " << added << std::endl;
    std::cout << "Removed: " << removed << std::endl;
    std::cout << "Contains: " << contains << std::endl;
    std::cout << "Removed + Size: " << removed + set.size() << std::endl;
    std::cout << std::endl;

    RandomGenerator generator(0, values.size()-1);
    for(int i=0; i<5; ++i){
        int random_number = generator();
        std::cout << "Does set contains " << values.at(random_number) << ": " << set.search(CustomType(random_number)) << std::endl;
        std::cout << "Remove call result: " << set.remove(CustomType(random_number)) << std::endl;
        std::cout << "Add call result: " << set.insert(CustomType(random_number)) << std::endl;
    }
    std::cout << std::endl;

    for(int i=0; i<5; ++i){
        int random_number = generator();
        std::cout << "Does set contains " << values.at(random_number) << ": " << set.search(CustomType(random_number)) << std::endl;
        std::cout << "Add call result: " << set.insert(CustomType(random_number)) << std::endl;
        std::cout << "Remove call result: " << set.remove(CustomType(random_number)) << std::endl;
    }

    std::cout << "Cleaning..." << std::endl;
    set.clear();
    std::cout << "Size: " << set.size() << std::endl;

    std::cout << "Done!" << std::endl;

    return 0;
}