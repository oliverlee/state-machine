#include "state_machine/variant2.h"

#include <iostream>

namespace {

template <class T0, class... Ts>
struct overload : T0, overload<Ts...> {
    using T0::operator();
    using overload<Ts...>::operator();

    overload(T0&& t0, Ts&&... ts)
        : T0{std::forward<T0>(t0)}, overload<Ts...>{std::forward<Ts>(ts)...} {}
};

template <class T>
struct overload<T> : T {
    using T::operator();

    overload(T&& t) : T{std::forward<T>(t)} {}
};

template <class... Ts>
auto make_overload(Ts&&... ts) -> overload<Ts...> {
    return overload<Ts...>{std::forward<Ts>(ts)...};
}

} // namespace

int main() {
    using ::state_machine::variant2::variant;
    auto v = variant<int, double, char>{};

    v.emplace<double>(3.14);

    v.visit(make_overload([](int& x) { std::cout << "int " << x << std::endl; },
                          [](double& x) { std::cout << "double " << x << std::endl; },
                          [](char& x) { std::cout << "char " << x << std::endl; }));
}
