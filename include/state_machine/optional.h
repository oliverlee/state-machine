#pragma once

#include <type_traits>

namespace state_machine {
namespace optional {

class bad_optional_access : public std::exception {};

struct nullopt_t {
    explicit constexpr nullopt_t(int) {}
};

static constexpr nullopt_t nullopt{0};

// A dumb substitute for std::optional (C++17).
// This class only provides minimal functionality.
template <class T>
class optional_base {
  public:
    // clang-tidy complains that data_ is not initialized with `= default`
    // NOLINTNEXTLINE(modernize-use-equals-default)
    constexpr optional_base() noexcept {}

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr optional_base(nullopt_t) noexcept {}

    template <class... Args, std::enable_if_t<std::is_constructible<T, Args...>::value, int> = 0>
    constexpr explicit optional_base(Args&&... args) noexcept(noexcept(T{
        std::forward<Args>(args)...}))
        : data_{std::forward<Args>(args)...}, has_value_{true} {}

    template <class U = T>
    auto operator=(U&& value) -> optional_base& {
        has_value_ = false;

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        data_ = T{std::forward<U>(value)};

        has_value_ = true;
    }

    auto operator=(nullopt_t) noexcept -> optional_base& { has_value_ = false; }

    constexpr auto has_value() const noexcept -> bool { return has_value_; }

    constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr auto operator->() const -> const T* {
        throw_if_empty();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        return &data_;
    }
    constexpr auto operator->() -> T* {
        throw_if_empty();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        return &data_;
    }
    constexpr auto operator*() const& -> const T& {
        throw_if_empty();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        return data_;
    }
    constexpr auto operator*() & -> T& {
        throw_if_empty();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        return data_;
    }
    constexpr auto operator*() const&& -> const T&& {
        throw_if_empty();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        return std::move(data_);
    }
    constexpr auto operator*() && -> T&& {
        throw_if_empty();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        return std::move(data_);
    }

    constexpr auto value() & -> T& { return **this; }

    constexpr auto value() const& -> const T& { return **this; }

    constexpr auto value() && -> T&& { return std::move(**this); }

    constexpr auto value() const&& -> const T&& { return std::move(**this); }

    template <class U>
    constexpr auto value_or(U&& default_value) const& -> T {
        return bool(*this) ? **this : static_cast<T>(std::forward<U>(default_value));
    }

    template <class U>
    constexpr auto value_or(U&& default_value) && -> T {
        return bool(*this) ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
    }

    void reset() noexcept { has_value_ = false; }

    template <class... Args>
    auto emplace(Args&&... args) -> T& {
        has_value_ = false;

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        data_ = T{std::forward<Args>(args)...};

        has_value_ = true;
    }

  private:
    inline void throw_if_empty() const {
        if (!bool(*this)) {
            throw bad_optional_access{};
        }
    }

    union {
        T data_;
        char dummy_ = {};
    };
    bool has_value_ = {false};
};


template <class T, class EnableDtor = void>
class optional : public optional_base<T> {
  public:
    using optional_base<T>::optional_base;

    // Default constructors/assignment operators will be noexcept if possible.

    // NOLINTNEXTLINE(performance-noexcept-move-constructor)
    optional(optional&&) = default;

    optional(const optional&) = default;

    // NOLINTNEXTLINE(performance-noexcept-move-constructor)
    auto operator=(optional &&) -> optional& = default;

    auto operator=(const optional&) -> optional& = default;

    ~optional() { destroy(); }

    template <class U = T>
    auto operator=(U&& value) -> optional& {
        destroy();
        optional_base<T>::operator=(std::forward<U>(value));
    }

    auto operator=(nullopt_t) noexcept -> optional& {
        destroy();
        optional_base<T>::operator=(nullopt);
    }

    void reset() noexcept {
        destroy();
        optional_base<T>::reset();
    }

    template <class... Args>
    auto emplace(Args&&... args) -> T& {
        destroy();
        optional_base<T>::emplace(std::forward<Args>(args)...);
    }

  private:
    void destroy() {
        this->has_value_ = false;
        if (bool(*this)) {
            this->value().T::~T();
        }
    }
};


template <class T>
class optional<T, std::enable_if_t<std::is_pod<T>::value>> : public optional_base<T> {
  public:
    using optional_base<T>::optional_base;
};

} // namespace optional
} // namespace state_machine
