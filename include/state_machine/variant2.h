#pragma once

#include "state_machine/backport.h"
#include "state_machine/containers.h"
#include "state_machine/traits.h"
#include "state_machine/variant.h"

#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace state_machine {
namespace variant2 {

namespace op = containers::operations;
using containers::basic::list;
using variant::bad_variant_access;

template <std::size_t I>
struct in_place_index_t {
    explicit in_place_index_t() = default;
};


/// @brief Underlying storage for a variant
template <class T0, class... Ts>
struct storage {
    static constexpr std::size_t index = sizeof...(Ts);
    using element_type = T0;

    static_assert(index > 0, "");

    static constexpr bool trivially_destructible =
        stdx::conjunction<std::is_trivially_destructible<T0>,
                          std::is_trivially_destructible<Ts>...>::value;

    constexpr storage() noexcept : alternatives_{} {}

    template <class... Args>
    constexpr storage(in_place_index_t<index>, Args&&... args) noexcept(
        std::is_nothrow_constructible<element_type, Args...>::value)
        : value_{std::forward<Args>(args)...} {}

    constexpr auto get(in_place_index_t<index>) noexcept -> element_type& { return value_; }
    constexpr auto get(in_place_index_t<index>) const noexcept -> const element_type& {
        return value_;
    }

    template <class Callable>
    constexpr auto invoke_on(in_place_index_t<index>, Callable&& callable)
        -> decltype(std::declval<Callable>()(std::declval<element_type&>())) {
        return callable(value_);
    }
    template <class Callable>
    constexpr auto invoke_on(in_place_index_t<index>, Callable&& callable) const
        -> decltype(std::declval<Callable>()(std::declval<const element_type&>())) {
        return callable(value_);
    }

    template <std::size_t I>
    constexpr decltype(auto) get(in_place_index_t<I>) noexcept {
        return alternatives_.get(in_place_index_t<I>{});
    }
    template <std::size_t I>
    constexpr decltype(auto) get(in_place_index_t<I>) const noexcept {
        return alternatives_.get(in_place_index_t<I>{});
    }

    template <std::size_t I, class Callable>
    constexpr auto invoke_on(in_place_index_t<I>, Callable&& callable)
        -> decltype(std::declval<Callable>()(std::declval<element_type&>())) {
        return alternatives_.invoke_on(in_place_index_t<I>{}, std::forward<Callable>(callable));
    }
    template <std::size_t I, class Callable>
    constexpr auto invoke_on(in_place_index_t<I>, Callable&& callable) const
        -> decltype(std::declval<Callable>()(std::declval<const element_type&>())) {
        return alternatives_.invoke_on(in_place_index_t<I>{}, std::forward<Callable>(callable));
    }

  private:
    union {
        element_type value_;
        storage<Ts...> alternatives_;
    };
};

template <class T>
struct storage<T> {
    static constexpr std::size_t index = 0;
    using element_type = T;

    static constexpr bool trivially_destructible = std::is_trivially_destructible<T>::value;

    constexpr storage() noexcept(std::is_nothrow_constructible<T>::value) : value_{} {}

    template <class... Args>
    constexpr storage(in_place_index_t<index>, Args&&... args) noexcept(
        std::is_nothrow_constructible<element_type, Args...>::value)
        : value_{std::forward<Args>(args)...} {}

    constexpr auto get(in_place_index_t<index>) noexcept -> element_type& { return value_; }
    constexpr auto get(in_place_index_t<index>) const noexcept -> const element_type& {
        return value_;
    }

    template <class Callable>
    constexpr auto invoke_on(in_place_index_t<index>, Callable&& callable)
        -> decltype(std::declval<Callable>()(std::declval<element_type&>())) {
        return callable(value_);
    }
    template <class Callable>
    constexpr auto invoke_on(in_place_index_t<index>, Callable&& callable) const
        -> decltype(std::declval<Callable>()(std::declval<const element_type&>())) {
        return callable(value_);
    }

  private:
    union {
        element_type value_;
        char dummy_;
    };
};


/// @brief A helper type for creating overloaded visitors
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

template <class T, class Tuple>
struct Index;

template <class T, class... Types>
struct Index<T, std::tuple<T, Types...>> {
    static const std::size_t value = 0;
};

template <class T, class U, class... Types>
struct Index<T, std::tuple<U, Types...>> {
    static const std::size_t value = 1 + Index<T, std::tuple<Types...>>::value;
};

template <class Derived, class Storage, class = void>
struct storage_with_destructor : Storage {
    using Storage::Storage;

    ~storage_with_destructor() noexcept { static_cast<Derived*>(this)->destroy_alternative(); }
};

template <class Derived, class Storage>
struct storage_with_destructor<Derived, Storage, std::enable_if_t<Storage::trivially_destructible>>
    : Storage {
    using Storage::Storage;
};

template <class... Ts>
class variant {
    using type = variant;
    using index_type = uint8_t;
    using storage_type =
        storage_with_destructor<variant, op::repack<op::reverse<list<Ts...>>, storage>>;
    using alternative_types = std::tuple<Ts...>;

    storage_type storage_ = {};
    index_type index_ = 0;

  public:
    // Number of alternative types
    static constexpr size_t size = sizeof...(Ts);

    static_assert(stdx::conjunction<aux::is_copy_or_move_constructible<Ts>...>::value,
                  "variant can only contain types that are copy or move constructible.");
    static_assert(stdx::conjunction<std::is_destructible<Ts>...>::value,
                  "ariant can only contain types that are destructible.");
    static_assert(!stdx::disjunction<std::is_reference<Ts>...>::value,
                  "variant cannot contain reference types.");
    static_assert(!stdx::disjunction<std::is_array<Ts>...>::value,
                  "variant cannot contain array types.");
    static_assert(sizeof...(Ts) <= std::numeric_limits<variant::index_type>::max(),
                  "Number of template type parameters exceeds variant maximum.");

  private:
    template <class T>
    static constexpr auto alternative_index() noexcept -> index_type {
        return Index<T, alternative_types>::value;
    }

    template <class Self, class Callable, std::size_t I0>
    static auto on_alternate_impl(Self self, Callable&& callable, std::index_sequence<I0>) {
        return self->storage_.invoke_on(in_place_index_t<0>{}, std::forward<Callable>(callable));
    }

    template <class Self, class Callable, std::size_t I0, std::size_t I1, std::size_t... Is>
    static auto
    on_alternate_impl(Self self, Callable&& callable, std::index_sequence<I0, I1, Is...>) {
        constexpr auto I = sizeof...(Is) + 1;

        return (self->index() == I) ? self->storage_.invoke_on(in_place_index_t<I>{},
                                                               std::forward<Callable>(callable)) :
                                      on_alternate_impl(self,
                                                        std::forward<Callable>(callable),
                                                        std::make_index_sequence<I>{});
    }

    template <class Self, class Callable>
    static auto on_alternate(Self self, Callable&& callable) {
        return on_alternate_impl(
            self, std::forward<Callable>(callable), std::make_index_sequence<size>{});
    }

    auto destroy_alternative() noexcept {
        on_alternate(this, [](auto& v) {
            using T = std::remove_reference_t<decltype(v)>;
            v.~T();
        });
    }

    template <class T, class Self>
    static constexpr auto get_if_impl(Self self)
        -> std::conditional_t<std::is_const<std::remove_pointer_t<Self>>::value, const T*, T*> {
        constexpr auto I = alternative_index<T>();

        if (self->index() != I) {
            return nullptr;
        }

        return &self->storage_.get(in_place_index_t<I>{});
    }


    template <class T, class Self>
    static constexpr auto get_impl(Self self)
        -> std::conditional_t<std::is_const<std::remove_pointer_t<Self>>::value, const T&, T&> {
        auto* p = get_if_impl<T>(self);

        if (p == nullptr) {
            throw bad_variant_access{};
        }

        return *p;
    }

  public:
    constexpr variant() = default;

    constexpr auto index() const noexcept -> index_type { return index_; }

    template <class T>
    constexpr auto holds_alternative() const noexcept -> bool {
        return alternative_index<T>() == index();
    }

    template <class T>
    constexpr auto get() & -> T& {
        return get_impl<T>(this);
    }
    template <class T>
    constexpr auto get() const& -> const T& {
        return get_impl<T>(this);
    }
    template <class T>
    constexpr auto get() && -> T&& {
        return std::move(get<T>());
    }
    template <class T>
    constexpr auto get() const&& -> const T&& {
        return std::move(get<T>());
    }

    template <class T, class... Args>
    auto emplace(Args&&... args) -> T& {
        if (holds_alternative<T>()) {
            return get<T>() = T{std::forward<Args>(args)...};
        }

        destroy_alternative();
        index_ = alternative_index<T>();
        return *(new (&storage_) T{std::forward<Args>(args)...});
    }

    template <class Callable>
    constexpr auto visit(Callable&& callable) {
        return on_alternate(this, std::forward<Callable>(callable));
    }
    template <class Callable>
    constexpr auto visit(Callable&& callable) const {
        return on_alternate(this, std::forward<Callable>(callable));
    }

    template <class... Overloads, class = std::enable_if_t<(sizeof...(Overloads) > 1)>>
    constexpr auto visit(Overloads&&... overloads) {
        return visit(make_overload(std::forward<Overloads>(overloads)...));
    }
    template <class... Overloads, class = std::enable_if_t<(sizeof...(Overloads) > 1)>>
    constexpr auto visit(Overloads&&... overloads) const {
        return visit(make_overload(std::forward<Overloads>(overloads)...));
    }
};

} // namespace variant2
} // namespace state_machine
