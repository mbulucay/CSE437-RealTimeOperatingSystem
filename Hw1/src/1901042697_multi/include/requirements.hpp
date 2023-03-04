#ifndef REQUIREMENTS_HPP__
#define REQUIREMENTS_HPP__

#include <type_traits>
#include <concepts>
#include <iostream>

template<class L, class R = L>
concept has_less_than = requires(const L& lhs, const R& rhs)
{
    {lhs < rhs} -> std::same_as<bool>;
};

template<class L, class R = L>
concept has_equal_to = requires(const L& lhs, const R& rhs)
{
    {lhs == rhs} -> std::same_as<bool>;
};

template<class L>
concept has_cout = requires(const L& lhs)
{
    {std::cout << lhs} -> std::same_as<std::ostream&>;
};


#endif // !REQUIREMENTS_HPP__