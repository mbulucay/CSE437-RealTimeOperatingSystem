#ifndef CUSTOM_TYPE_HPP__
#define CUSTOM_TYPE_HPP__

#include <iostream>

class CustomType{
public:
    int x;
    CustomType(int x);

    bool operator==(const CustomType& other) const;

    bool operator<(const CustomType& other)  const;

    friend std::ostream& operator<<(std::ostream& os, const CustomType& obj){
        os << obj.x;
        return os;
    }
};

#endif // !CUSTOM_TYPE_HPP__