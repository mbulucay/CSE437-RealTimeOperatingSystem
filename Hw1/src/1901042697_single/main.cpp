#include <iostream>
#include <thread>
#include <random>
#include <iomanip>
#include <algorithm>
#include <chrono>

#include "./include/custom_type.hpp"
#include "./include/thread_safe_set.hpp"
#include "./include/random_generator.hpp"

int main(){

    constexpr int SIZE = 100000;
    std::vector<int> values(2*SIZE+1);
    std::iota(begin(values), end(values), 0);
    std::random_shuffle(begin(values), end(values));
    values.resize(SIZE);

    mbu::ThreadSafeSet<CustomType> set;
    std::thread writerThread, removeThread, readerThread;

    auto start = std::chrono::high_resolution_clock::now();

    int added = 0;
    int removed = 0;
    writerThread = std::thread([&](){

        for (int i = 0; i < SIZE/3; ++i) {
            if(set.insert(CustomType(values[i]))){
                ++added;
            }
        }

        for(int i = SIZE/3; i < SIZE/5; ++i){
            CustomType value(values[i]);
            if(set.insert(std::move(value))){
                ++added;
            }
        }

        for(int i = SIZE/5; i < SIZE; ++i){
            CustomType value(values[i]);
            if(set.insert(value)){
                ++added;
            }
        }

        for(int i = 0; i < SIZE; ++i){
            if(i % 2 == 0 && set.remove(CustomType(values[i]))){
                ++removed;
            }
        }

    });

    /* 
        if you want to increase the chance of exist in the numbers in tree increase the number
        but at some point half of the numbers will be removed and the max number will be half of the size
        1 - 300 millisecond you could try for size 100000
    */
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // std::this_thread::sleep_for(std::chrono::nanoseconds(20));

    int contains = 0;
    readerThread = std::thread([&](){
        for (int i = 0; i < SIZE; ++i) {
            if(set.search(CustomType(values[i]))){
                ++contains;
            }
        }
    });

    writerThread.join();
    readerThread.join();

    auto end = std::chrono::high_resolution_clock::now();

    set.iterate([](const CustomType& obj){
        std::cout << obj << ", ";
    });

    std::cout << std::endl;

    std::cout << "Insertion time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
    std::cout << "Size: " << set.size() << std::endl;
    std::cout << "Added: " << added << std::endl;
    std::cout << "Removed: " << removed << std::endl;
    std::cout << "Contains: " << contains << std::endl;
    std::cout << "Removed + Size: " << removed + set.size() << std::endl;

    std::cout << std::endl;
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