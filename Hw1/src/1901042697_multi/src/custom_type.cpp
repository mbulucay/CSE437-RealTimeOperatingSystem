#include "../include/custom_type.hpp"


CustomType::CustomType(int x) : x(x) {
}


bool CustomType::operator==(const CustomType& other) const {
    return x == other.x;
}


bool CustomType::operator<(const CustomType& other) const {
    return x < other.x;
}