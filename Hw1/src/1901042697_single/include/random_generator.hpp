#ifndef RANDOM_GENERATOR_HPP_
#define RANDOM_GENERATOR_HPP_

#include <random>

class RandomGenerator {

    public:
        RandomGenerator(int lower, int upper) 
            : eng(std::random_device()())
            , distr(lower, upper)
        {}

        int operator()() { 
            return distr(eng);
        }

    private:
        std::mt19937 eng;
        std::uniform_int_distribution<> distr;
};


#endif