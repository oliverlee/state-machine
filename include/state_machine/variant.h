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
namespace op = ::state_machine::containers::op;

// The type of exception thrown if `Variant::get` is called with the wrong type.
class bad_variant_access : public std::exception {};

// An "empty" type that is held by a Variant on initialization and alternative
// types are taken.
struct empty {};

template <class... Ts>
class Variant {
  public:
    using alternative_index_map = index_map<empty, Ts...>;
    using index_type = uint8_t;

    // Number of alternative types, excluding the "empty" type.
    static constexpr size_t size = sizeof...(Ts);

  private:
    using type = Variant<Ts...>;
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
    constexpr Variant() noexcept = default;

    Variant(const Variant&) = delete;
    Variant& operator=(const Variant&) = delete;

    Variant(Variant&& rhs) noexcept(
        stdx::conjunction<std::is_nothrow_move_constructible<Ts>...>::value) {
        if (!rhs.holds<empty>()) {
            rhs.visit([this](auto&& s) {
                this->set<std::remove_reference_t<decltype(s)>>(std::move(s));
            });
        }
    }

    auto operator=(Variant&& rhs) noexcept(
        stdx::conjunction<std::is_nothrow_move_assignable<Ts>...>::value) -> Variant& {
        if (rhs.holds<empty>()) {
            this->emplace<empty>();
        } else {
            // A double move is used to handle self-assignment.
            auto temp = Variant{std::move(rhs)};
            temp.visit([this](auto&& s) { this->set(std::move(s)); });
        }
        return *this;
    }

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
        index_ = alternative_index<D>();
        return *(new (std::addressof(storage_)) D{std::forward<T>(t)});
    }

    template <class T,
              class D = std::remove_reference_t<T>,
              std::enable_if_t<alternative_index_map::template contains_key<D>::value &&
                                   !std::is_move_constructible<D>::value,
                               int> = 0>
    auto set(const T& t) noexcept(noexcept(std::declval<Variant>().destroy_internal()) &&
                                  std::is_nothrow_copy_constructible<D>::value) -> D& {
        destroy_internal();
        index_ = alternative_index<D>();
        return *(new (std::addressof(storage_)) D{t});
    }

    template <class T, class... Args, enable_if_key_t<T> = 0>
    auto emplace(Args&&... args) noexcept(noexcept(
        std::declval<Variant>().destroy_internal()) && noexcept(T{std::forward<Args>(args)...}))
        -> T& {
        destroy_internal();
        index_ = alternative_index<T>();
        return *(new (std::addressof(storage_)) T{std::forward<Args>(args)...});
    }

    template <class T, enable_if_key_t<T> = 0>
    constexpr auto holds() const noexcept -> bool {
        return alternative_index<T>() == index();
    }

    template <class T, enable_if_key_t<T> = 0>
    auto get() -> T& {
        auto* state = get_impl<T>();

        if (state == nullptr) {
            throw bad_variant_access{};
        }

        return *state;
    }

    template <class T, enable_if_key_t<T> = 0>
    auto get() const -> const T& {
        return get();
    }

    template <class T, enable_if_key_t<T> = 0>
    auto get_if() noexcept -> T* {
        return get_impl<T>();
    }

    template <class T, enable_if_key_t<T> = 0>
    auto get_if() const noexcept -> const T* {
        return get_impl<T>();
    }

    template <class T,
              std::enable_if_t<alternative_index_map::template contains_key<T>::value &&
                                   std::is_move_constructible<T>::value,
                               int> = 0>
    auto take() -> T {
        auto retval = std::move(get<T>());
        emplace<empty>();
        return retval;
    }

    template <class T,
              std::enable_if_t<alternative_index_map::template contains_key<T>::value &&
                                   !std::is_move_constructible<T>::value,
                               int> = 0>
    auto take() -> const T {
        const auto retval = get<T>();
        emplace<empty>();
        // Return a const value to force binding to the object copy constructor.
        return retval;
    }

    template <class Callable>
    auto visit(Callable callable) {
        static_assert(sizeof...(Ts) > 0,
                      "`visit` cannot be called if Variant is not defined with any alternatives.");

        if (holds<empty>()) {
            throw bad_variant_access{};
        }

        return on_alternate<op::pop_front<op::repack<alternative_index_map, bijection>>>::invoke(
            *this, callable);
    }

    constexpr auto index() const noexcept -> index_type { return index_; }

    template <class T, enable_if_key_t<T> = 0>
    static constexpr auto alternative_index() noexcept {
        return static_cast<index_type>(alternative_index_map::template at_key<T>::value);
    }

  private:
    template <class Mapping>
    struct on_alternate;

    template <class Entry>
    struct on_alternate<bijection<Entry>> {

        template <class Callable>
        static constexpr auto invoke(type& self, const Callable& callable) {
            if (self.index() != Entry::second_type::value) {
                throw bad_variant_access{};
            }
            using T = typename Entry::first_type;
            return callable(self.get<T>());
        }
    };

    template <class Entry, class Next, class... Rest>
    struct on_alternate<bijection<Entry, Next, Rest...>>
        : private on_alternate<bijection<Next, Rest...>> {

        template <class Callable>
        static constexpr auto invoke(type& self, const Callable& callable) {
            using T = typename Entry::first_type;
            return (self.index() == Entry::second_type::value) ?
                       callable(self.get<T>()) :
                       on_alternate<bijection<Next, Rest...>>::invoke(self, callable);
        }
    };

    inline auto
    destroy_internal() noexcept(stdx::disjunction<std::is_nothrow_destructible<Ts>...>::value)
        -> void {
        return on_alternate<alternative_index_map>::invoke(*this, [](auto&& s) {
            using T = std::decay_t<decltype(s)>;
            s.T::~T();
        });
    }

    template <class T, enable_if_key_t<T> = 0>
    inline auto get_impl() noexcept -> T* {
        if (holds<T>()) {
            return reinterpret_cast<T*>(std::addressof(storage_));
        }
        return nullptr;
    }

    storage_type storage_;
    index_type index_ = alternative_index_map::template at_key<empty>::value;
};

} // namespace variant
} // namespace state_machine
