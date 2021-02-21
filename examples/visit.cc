#include "state_machine/variant2.h"

#include <iostream>

int main() {
    using ::state_machine::variant2::variant;
    auto v = variant<int, double, char>{};

    std::cout << "holds double? " << v.holds_alternative<double>() << std::endl;
    std::cout << "index " << v.index() << std::endl;

    v.emplace<double>(3.14);

    v.visit([](int& x) { std::cout << "int " << x << std::endl; },
            [](double& x) { std::cout << "double " << x << std::endl; },
            [](char& x) { std::cout << "char " << x << std::endl; });
}
