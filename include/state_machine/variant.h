#pragma once

#include "state_machine/backport.h"
#include "state_machine/containers.h"
#include "state_machine/traits.h"

#include <limits>
#include <stdexcept>
#include <type_traits>

namespace state_machine {
namespace variant {

using ::state_machine::containers::bijection;
using ::state_machine::containers::index_map;

// The type of exception thrown if `Variant::get` is called with the wrong type.
class bad_variant_access : public std::exception {};

// An "empty" type that is held by a Variant on initialization and alternative
// types are taken.
struct empty {};

template <class... Ts>
class Variant {
    using alternative_index_map = index_map<empty, Ts...>;
    using index_type = uint8_t;
    using storage_type = std::aligned_union_t<0, empty, Ts...>;

    static_assert(stdx::conjunction<aux::is_copy_or_move_constructible<Ts>...>::value,
                  "Variant can only contain types that are copy or move constructible.");
    static_assert(stdx::conjunction<std::is_destructible<Ts>...>::value,
                  "Variant can only contain types that are destructible.");
    static_assert(!stdx::disjunction<std::is_reference<Ts>...>::value,
                  "Variant cannot contain reference types.");
    static_assert(!stdx::disjunction<std::is_array<Ts>...>::value,
                  "Variant cannot contain array types.");
    static_assert(sizeof...(Ts) < std::numeric_limits<Variant::index_type>::max(),
                  "Number of template type parameters exceeds Variant maximum.");

    template <class T>
    using enable_if_key_t =
        std::enable_if_t<alternative_index_map::template contains_key<T>::value, int>;

  public:
    constexpr Variant() noexcept : index_{alternative_index_map::template at_key<empty>::value} {}

    Variant(const Variant&) = delete;
    Variant& operator=(const Variant&) = delete;
    Variant(Variant&&) = delete;
    Variant& operator=(Variant&& other) = delete;

    ~Variant() noexcept(noexcept(std::declval<Variant>().destroy_internal())) {
        destroy_internal();
    }

    template <class T,
              class D = std::remove_reference_t<T>,
              std::enable_if_t<alternative_index_map::template contains_key<D>::value &&
                                   std::is_move_constructible<D>::value,
                               int> = 0>
    auto set(T&& t) noexcept(noexcept(std::declval<Variant>().destroy_internal()) &&
                             std::is_nothrow_move_constructible<D>::value) -> D& {
        destroy_internal();
        index_ = index<D>();
        return *(new (static_cast<void*>(std::addressof(storage_))) D{std::forward<T>(t)});
    }

    template <class T,
              class D = std::remove_reference_t<T>,
              std::enable_if_t<alternative_index_map::template contains_key<D>::value &&
                                   !std::is_move_constructible<D>::value,
                               int> = 0>
    auto set(const T& t) noexcept(noexcept(std::declval<Variant>().destroy_internal()) &&
                                  std::is_nothrow_copy_constructible<D>::value) -> D& {
        destroy_internal();
        index_ = index<D>();
        return *(new (static_cast<void*>(std::addressof(storage_))) D{t});
    }

    template <class T, class... Args, enable_if_key_t<T> = 0>
    auto emplace(Args&&... args) noexcept(noexcept(std::declval<Variant>().destroy_internal()) &&
                                          std::is_nothrow_constructible<T>::value) -> T& {
        destroy_internal();
        index_ = index<T>();
        return *(new (static_cast<void*>(std::addressof(storage_))) T{std::forward<Args>(args)...});
    }

    template <class T, enable_if_key_t<T> = 0>
    constexpr auto holds() const noexcept -> bool {
        return index<T>() == index_;
    }

    template <class T, enable_if_key_t<T> = 0>
    auto get() -> T& {
        if (index<T>() != index_) {
            throw bad_variant_access{};
        }
        return *reinterpret_cast<T*>(std::addressof(storage_));
    }

    template <class T, enable_if_key_t<T> = 0>
    auto get() const -> const T& {
        if (index<T>() != index_) {
            throw bad_variant_access{};
        }
        return *reinterpret_cast<T*>(std::addressof(storage_));
    }

    template <class T, enable_if_key_t<T> = 0>
    auto get_if() noexcept -> T* {
        if (index<T>() != index_) {
            return nullptr;
        }
        return reinterpret_cast<T*>(std::addressof(storage_));
    }

    template <class T, enable_if_key_t<T> = 0>
    auto get_if() const noexcept -> const T* {
        if (index<T>() != index_) {
            return nullptr;
        }
        return reinterpret_cast<T*>(std::addressof(storage_));
    }

    template <class T,
              std::enable_if_t<alternative_index_map::template contains_key<T>::value &&
                                   std::is_move_constructible<T>::value,
                               int> = 0>
    auto take() -> T {
        if (index<T>() != index_) {
            throw bad_variant_access{};
        }
        auto retval = std::move(*reinterpret_cast<T*>(std::addressof(storage_)));
        emplace<empty>();
        return retval;
    }

    template <class T,
              std::enable_if_t<alternative_index_map::template contains_key<T>::value &&
                                   !std::is_move_constructible<T>::value,
                               int> = 0>
    auto take() -> const T {
        if (index<T>() != index_) {
            throw bad_variant_access{};
        }
        const auto retval = *reinterpret_cast<T*>(std::addressof(storage_));
        emplace<empty>();
        // Return a const value to force binding to the object copy constructor.
        return retval;
    }

    template <class Callable>
    auto visit(Callable callable) {
        if (holds<empty>()) {
            throw bad_variant_access{};
        }

        // Reduce index_ since the first 'empty' type is never visited.
        const auto without_empty_index = [](index_type index) -> index_type {
            return static_cast<index_type>(index - 1);
        };

        return on_alternate<index_map<Ts...>>::invoke(
            without_empty_index(index_), storage_, callable);
    }

  private:
    template <class M>
    struct on_alternate;

    template <class Entry>
    struct on_alternate<bijection<Entry>> {
        static constexpr auto type_destructor(index_type index) noexcept {
            using T = typename Entry::first_type;
            return (index == Entry::second_type::value) ?
                   +[](void* storage) -> void { static_cast<T*>(storage)->~T(); } :
                   nullptr;
        }

        template <class Callable>
        static constexpr auto
        invoke(index_type index, storage_type& storage, const Callable& callable) {
            if (index != Entry::second_type::value) {
                throw bad_variant_access{};
            }
            using T = typename Entry::first_type;
            return callable(reinterpret_cast<T&>(storage));
        }
    };

    template <class Entry, class... Entries>
    struct on_alternate<bijection<Entry, Entries...>>
        : private on_alternate<bijection<Entries...>> {
        static constexpr auto type_destructor(index_type index) noexcept {
            using T = typename Entry::first_type;
            return (index == Entry::second_type::value) ?
                   +[](void* storage) -> void { static_cast<T*>(storage)->~T(); } :
                   on_alternate<bijection<Entries...>>::type_destructor(index);
        }

        template <class Callable>
        static constexpr auto
        invoke(index_type index, storage_type& storage, const Callable& callable) {
            using T = typename Entry::first_type;
            return (index == Entry::second_type::value) ?
                       callable(reinterpret_cast<T&>(storage)) :
                       on_alternate<bijection<Entries...>>::invoke(index, storage, callable);
        }
    };

    template <class T, enable_if_key_t<T> = 0>
    static constexpr auto index() noexcept {
        return static_cast<index_type>(alternative_index_map::template at_key<T>::value);
    }

    auto destroy_internal() noexcept(stdx::disjunction<std::is_nothrow_destructible<Ts>...>::value)
        -> void {
        // Given `index_` corresponds to a value in `alternative_index_map`,
        // this lookup should always succeed.
        auto destroy = on_alternate<alternative_index_map>::type_destructor(index_);
        (*destroy)(static_cast<void*>(std::addressof(storage_)));
    }

    storage_type storage_;
    index_type index_;
};

} // namespace variant
} // namespace state_machine
