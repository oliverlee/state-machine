#include "state_machine/variant2.h"

#include <iostream>

int main() {
    using ::state_machine::variant2::in_place_index;
    using ::state_machine::variant2::variant;
    auto v = variant<int, double, char>{};

    std::cout << "holds double? " << v.holds_alternative<double>() << std::endl;
    std::cout << "index " << v.index() << std::endl;

    v.emplace<double>(3.14);

    v.visit([](int& x) { std::cout << "int " << x << std::endl; },
            [](double& x) { std::cout << "double " << x << std::endl; },
            [](char& x) { std::cout << "char " << x << std::endl; });

    constexpr auto v2 = variant<double, char>{};
    constexpr auto d = v2.get<double>();

    std::cout << "d: " << d << std::endl;

    {
        constexpr auto v3 = variant<char, double>{in_place_index<1>, 3.14};
        std::cout << "holds double? " << v3.holds_alternative<double>() << std::endl;
        std::cout << "double: " << v3.get<double>() << std::endl;
    }

    {
        constexpr auto v3 = variant<double, char>{in_place_index<0>, 3.14};
        std::cout << "holds double? " << v3.holds_alternative<double>() << std::endl;
        std::cout << "double: " << v3.get<double>() << std::endl;
    }

    {
        constexpr auto v3 = variant<char, double>{3.14};
        std::cout << "holds double? " << v3.holds_alternative<double>() << std::endl;
        std::cout << "double: " << v3.get<double>() << std::endl;
    }

    {
        constexpr auto v3 = variant<char, double>{3.14F};
        std::cout << "holds double? " << v3.holds_alternative<double>() << std::endl;
        std::cout << "double: " << v3.get<double>() << std::endl;
    }
}
