#pragma once

#include "state_machine/backport.h"
#include "state_machine/containers.h"
#include "state_machine/traits.h"
#include "state_machine/variant.h"

#include <limits>
#include <stdexcept>
#include <type_traits>

namespace state_machine {
namespace variant2 {

using ::state_machine::containers::bijection;
using ::state_machine::containers::index_map;
namespace op = ::state_machine::containers::op;

using variant::bad_variant_access;

template <class... Ts>
class variant {
    using index_type = uint8_t;

  public:
    using alternative_map = index_map<Ts...>;

    // Number of alternative types
    static constexpr size_t size = sizeof...(Ts);

    static_assert(stdx::conjunction<aux::is_copy_or_move_constructible<Ts>...>::value,
                  "Variant can only contain types that are copy or move constructible.");
    static_assert(stdx::conjunction<std::is_destructible<Ts>...>::value,
                  "Variant can only contain types that are destructible.");
    static_assert(!stdx::disjunction<std::is_reference<Ts>...>::value,
                  "Variant cannot contain reference types.");
    static_assert(!stdx::disjunction<std::is_array<Ts>...>::value,
                  "Variant cannot contain array types.");
    static_assert(sizeof...(Ts) <= std::numeric_limits<variant::index_type>::max(),
                  "Number of template type parameters exceeds Variant maximum.");

  private:
    using type = variant;
    using storage_type = std::aligned_union_t<0, Ts...>;

    template <class T, class R = void>
    using enable_if_key_t = std::enable_if_t<alternative_map::template contains_key<T>::value, R>;

    using T0 = typename alternative_map::template at_value<aux::index_constant<0>>;

    storage_type storage_ = {};
    index_type index_ = 0;

    template <class T>
    static constexpr auto alternative_index() noexcept -> index_type {
        return static_cast<index_type>(alternative_map::template at_key<T>::value);
    }

    template <class T, class Self>
    static auto get_if_impl(Self self) noexcept
        -> std::conditional_t<std::is_const<std::remove_pointer_t<Self>>::value, const T*, T*> {
        if (self->template holds_alternative<T>()) {
            using C =
                std::conditional_t<std::is_const<std::remove_pointer_t<Self>>::value, const T, T>;
            return reinterpret_cast<C*>(&self->storage_);
        }

        return nullptr;
    }
    template <std::size_t I, class Self>
    static auto get_if_impl(Self self) noexcept {
        using T = typename alternative_map::template at_value<aux::index_constant<I>>;
        return get_if_impl<T>(self);
    }

    template <class T, class Self>
    static decltype(auto) get_impl(Self self) {
        auto* p = get_if_impl<T>(self);

        if (p == nullptr) {
            throw bad_variant_access{};
        }

        return *p;
    }
    template <std::size_t I, class Self>
    static decltype(auto) get_impl(Self self) {
        using T = typename alternative_map::template at_value<aux::index_constant<I>>;
        return get_impl<T>(self);
    }

    template <class Self, class Callable, std::size_t I0>
    static auto on_alternate_impl(Self self, Callable&& callable, std::index_sequence<I0>) {
        return callable(*get_if_impl<0>(self));
    }
    template <class Self, class Callable, std::size_t I0, std::size_t I1, std::size_t... Is>
    static auto
    on_alternate_impl(Self self, Callable&& callable, std::index_sequence<I0, I1, Is...>) {
        constexpr auto i = sizeof...(Is) + 1;

        auto* p = get_if_impl<i>(self);

        return (p != nullptr) ? callable(*p) :
                                on_alternate_impl<Self, Callable>(self,
                                                                  std::forward<Callable>(callable),
                                                                  std::make_index_sequence<i>{});
    }

    template <class Self, class Callable>
    static auto on_alternate(Self self, Callable&& callable) {
        return on_alternate_impl(
            self, std::forward<Callable>(callable), std::make_index_sequence<size>{});
    }

    auto destroy_alternative() noexcept -> void {
        on_alternate(this, [](auto& v) {
            using T = std::remove_reference_t<decltype(v)>;
            v.~T();
        });
    }

  public:
    variant() noexcept(std::is_nothrow_default_constructible<T0>::value) : index_{0} {
        (void)(new (&storage_) T0{});
    }

    ~variant() noexcept { destroy_alternative(); }

    constexpr auto index() const noexcept -> index_type { return index_; }

    template <class T>
    auto holds_alternative() const noexcept -> enable_if_key_t<T, bool> {
        return alternative_index<T>() == index();
    }

    template <class T>
    auto get() & -> enable_if_key_t<T, T&> {
        return get_impl<T>(this);
    }
    template <class T>
    auto get() const& -> enable_if_key_t<T, const T&> {
        return get_impl<T>(this);
    }
    template <class T>
    auto get() && -> enable_if_key_t<T, T&&> {
        return std::move(get<T>());
    }
    template <class T>
    auto get() const&& -> enable_if_key_t<T, const T&&> {
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
    auto visit(Callable&& callable) {
        return on_alternate(this, std::forward<Callable>(callable));
    }
    template <class Callable>
    auto visit(Callable&& callable) const {
        return on_alternate(this, std::forward<Callable>(callable));
    }
};

} // namespace variant2
} // namespace state_machine
