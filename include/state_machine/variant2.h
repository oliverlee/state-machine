#pragma once

#include "state_machine/backport.h"
#include "state_machine/containers.h"
#include "state_machine/traits.h"
#include "state_machine/variant.h"

#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace state_machine {
namespace variant2 {

namespace op = containers::operations;
using containers::basic::list;
using variant::bad_variant_access;

template <std::size_t I>
struct in_place_index_t {
    explicit in_place_index_t() = default;
};
template <std::size_t I>
constexpr in_place_index_t<I> in_place_index{};

template <class T>
struct is_in_place_index_t : std::false_type {};
template <std::size_t I>
struct is_in_place_index_t<in_place_index_t<I>> : std::true_type {};

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
struct index_of;

template <class T, class... Types>
struct index_of<T, std::tuple<T, Types...>> {
    static const std::size_t value = 0;

    static_assert(!stdx::disjunction<std::is_same<T, Types>...>::value, "");
};

template <class T, class U, class... Types>
struct index_of<T, std::tuple<U, Types...>> {
    static const std::size_t value = 1 + index_of<T, std::tuple<Types...>>::value;
};

template <class IndexedStorage, class Callable>
constexpr auto
invoke_on_alt_impl(IndexedStorage&& storage, Callable&& callable, std::index_sequence<0>) {
    if (storage.index() != 0) {
        throw bad_variant_access{};
    }

    return callable(std::forward<IndexedStorage>(storage).template get(in_place_index<0>));
}

template <class IndexedStorage, class Callable, std::size_t... Indices>
constexpr auto
invoke_on_alt_impl(IndexedStorage&& storage, Callable&& callable, std::index_sequence<Indices...>) {
    constexpr auto n = sizeof...(Indices) - 1;
    return (storage.index() == n) ?
               callable(std::forward<IndexedStorage>(storage).template get(in_place_index<n>)) :
               invoke_on_alt_impl(std::forward<IndexedStorage>(storage),
                                  std::forward<Callable>(callable),
                                  std::make_index_sequence<n>{});
}

template <class IndexedStorage, class Callable>
constexpr auto invoke_on_alternative(IndexedStorage&& storage, Callable&& callable) {
    return invoke_on_alt_impl(std::forward<IndexedStorage>(storage),
                              std::forward<Callable>(callable),
                              std::make_index_sequence<std::decay_t<IndexedStorage>::size>{});
}

template <class IndexedStorage>
auto destroy_alternative(IndexedStorage& storage) {
    invoke_on_alternative(storage, [](auto& v) {
        using T = std::remove_reference_t<decltype(v)>;
        v.~T();
    });
}

/// @brief Base class template providing get methods
template <class T, class Storage>
struct get_index {
    template <std::size_t Index, std::enable_if_t<(Index == 0), bool> = true>
    constexpr auto get(in_place_index_t<Index>) & noexcept -> T& {
        return static_cast<Storage&>(*this).data_;
    }
    template <std::size_t Index, std::enable_if_t<(Index == 0), bool> = true>
    constexpr auto get(in_place_index_t<Index>) const& noexcept -> const T& {
        return static_cast<const Storage&>(*this).data_;
    }

    template <std::size_t Index, std::enable_if_t<(Index > 0), bool> = true>
    constexpr decltype(auto) get(in_place_index_t<Index>) & noexcept {
        return static_cast<Storage&>(*this).alternatives_.template get(in_place_index<Index - 1>);
    }
    template <std::size_t Index, std::enable_if_t<(Index > 0), bool> = true>
    constexpr decltype(auto) get(in_place_index_t<Index>) const& noexcept {
        return static_cast<const Storage&>(*this).alternatives_.template get(
            in_place_index<Index - 1>);
    }

    template <std::size_t Index>
    constexpr decltype(auto) get(in_place_index_t<Index>) && noexcept {
        return std::move(get(in_place_index<Index>));
    }
    template <std::size_t Index>
    constexpr decltype(auto) get(in_place_index_t<Index>) const&& noexcept {
        return std::move(get(in_place_index<Index>));
    }
};

/// @brief Underlying storage bases for a variant
template <class... Ts>
struct nontrivial_storage {};

template <class T0, class... Ts>
struct nontrivial_storage<T0, Ts...> : get_index<T0, nontrivial_storage<T0, Ts...>> {
    template <class U = T0, class = std::enable_if_t<std::is_default_constructible<U>::value>>
    constexpr nontrivial_storage() noexcept(std::is_nothrow_default_constructible<U>::value)
        : data_{} {}

    ~nontrivial_storage() {}

    union {
        T0 data_;
        nontrivial_storage<Ts...> alternatives_;
    };
};

template <class... Ts>
struct trivial_storage {};

template <class T0, class... Ts>
struct trivial_storage<T0, Ts...> : get_index<T0, trivial_storage<T0, Ts...>> {
    template <class U = T0, class = std::enable_if_t<std::is_default_constructible<U>::value>>
    constexpr trivial_storage() noexcept(std::is_nothrow_default_constructible<U>::value)
        : data_{} {}

    union {
        T0 data_;
        trivial_storage<Ts...> alternatives_;
    };
};

template <bool TriviallyDestructible, class... Ts>
struct destructible_storage;

template <class... Ts>
struct destructible_storage<false, Ts...> : nontrivial_storage<Ts...> {
    using base_type = nontrivial_storage<Ts...>;
    using base_type::base_type;

    ~destructible_storage() { destroy_alternative(*this); }
};

template <class... Ts>
struct destructible_storage<true, Ts...> : trivial_storage<Ts...> {
    using base_type = trivial_storage<Ts...>;
    using base_type::base_type;
};

template <class... Ts>
struct storage
    : destructible_storage<stdx::conjunction<std::is_trivially_destructible<Ts>...>::value, Ts...> {
    using base_type =
        destructible_storage<stdx::conjunction<std::is_trivially_destructible<Ts>...>::value,
                             Ts...>;
    using base_type::base_type;

    static constexpr auto size = sizeof...(Ts);

    constexpr auto index() const noexcept { return index_; }

    std::size_t index_ = 0;
};

template <class... Ts>
class variant : storage<Ts...> {
    using type = variant;
    using base_type = storage<Ts...>;
    using alternative_types = std::tuple<Ts...>;

    using base_type::base_type;

  public:
    // Number of alternative types
    static constexpr size_t size = sizeof...(Ts);

    static_assert(stdx::conjunction<aux::is_copy_or_move_constructible<Ts>...>::value,
                  "variant can only contain types that are copy or move constructible.");
    static_assert(stdx::conjunction<std::is_destructible<Ts>...>::value,
                  "variant can only contain types that are destructible.");
    static_assert(!stdx::disjunction<std::is_reference<Ts>...>::value,
                  "variant cannot contain reference types.");
    static_assert(!stdx::disjunction<std::is_array<Ts>...>::value,
                  "variant cannot contain array types.");

    using base_type::index;

    template <std::size_t I>
    using alternative_type = std::tuple_element_t<I, alternative_types>;

  private:
    template <class T>
    static constexpr auto alternative_index() noexcept {
        return index_of<T, alternative_types>::value;
    }

    template <class T, class Self>
    static constexpr auto get_if_impl(Self self)
        -> std::conditional_t<std::is_const<std::remove_pointer_t<Self>>::value, const T*, T*> {
        constexpr auto I = alternative_index<T>();

        if (self->index() != I) {
            return nullptr;
        }

        using Base = std::conditional_t<std::is_const<std::remove_pointer_t<Self>>::value,
                                        const base_type,
                                        base_type>;
        return &static_cast<Base&>(*self).template get(in_place_index_t<I>{});
    }

    template <class T, class Self>
    static constexpr decltype(auto) get_impl(Self self) {
        auto* p = get_if_impl<T>(self);

        if (p == nullptr) {
            throw bad_variant_access{};
        }

        return *p;
    }

  public:
    template <class T>
    constexpr auto holds_alternative() const noexcept -> bool {
        return alternative_index<T>() == this->index();
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

        destroy_alternative(static_cast<base_type&>(*this));
        this->index_ = alternative_index<T>();
        return *(new (&(this->data_)) T{std::forward<Args>(args)...});
    }

    template <class Callable>
    constexpr decltype(auto) visit(Callable&& callable) {
        return invoke_on_alternative(static_cast<base_type&>(*this),
                                     std::forward<Callable>(callable));
    }
    template <class Callable>
    constexpr decltype(auto) visit(Callable&& callable) const {
        return invoke_on_alternative(static_cast<const base_type&>(*this),
                                     std::forward<Callable>(callable));
    }

    template <class... Overloads, class = std::enable_if_t<(sizeof...(Overloads) > 1)>>
    constexpr decltype(auto) visit(Overloads&&... overloads) {
        return visit(make_overload(std::forward<Overloads>(overloads)...));
    }
    template <class... Overloads, class = std::enable_if_t<(sizeof...(Overloads) > 1)>>
    constexpr decltype(auto) visit(Overloads&&... overloads) const {
        return visit(make_overload(std::forward<Overloads>(overloads)...));
    }
};

} // namespace variant2
} // namespace state_machine
