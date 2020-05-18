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
    constexpr optional_base() noexcept : dummy_{}, has_value_{false} {}
    constexpr optional_base(nullopt_t) noexcept : dummy_{}, has_value_{false} {}

    template <class... Args, std::enable_if_t<std::is_constructible<T, Args...>::value, int> = 0>
    constexpr explicit optional_base(Args&&... args) noexcept(noexcept(T{
        std::forward<Args>(args)...}))
        : data_{std::forward<Args>(args)...}, has_value_{true} {}

    template <class U = T>
    optional_base& operator=(U&& value) {
        has_value_ = false;
        data_ = T{std::forward<U>(value)};
        has_value_ = true;
    }

    optional_base& operator=(nullopt_t) noexcept { has_value_ = false; }

    constexpr bool has_value() const noexcept { return has_value_; }

    constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr const T* operator->() const {
        if (!bool(*this)) {
            throw bad_optional_access{};
        }
        throw_if_empty();
        return &data_;
    }
    constexpr T* operator->() {
        throw_if_empty();
        return &data_;
    }
    constexpr const T& operator*() const& {
        throw_if_empty();
        return data_;
    }
    constexpr T& operator*() & {
        throw_if_empty();
        return data_;
    }
    constexpr const T&& operator*() const&& {
        throw_if_empty();
        return std::move(data_);
    }
    constexpr T&& operator*() && {
        throw_if_empty();
        return std::move(data_);
    }

    constexpr T& value() & { return **this; }

    constexpr const T& value() const& { return **this; }

    constexpr T&& value() &&;
    constexpr const T&& value() const&& { return std::move(**this); }

    template <class U>
    constexpr T value_or(U&& default_value) const& {
        return bool(*this) ? **this : static_cast<T>(std::forward<U>(default_value));
    }

    template <class U>
    constexpr T value_or(U&& default_value) && {
        return bool(*this) ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
    }

    void reset() noexcept { has_value_ = false; }

    template <class... Args>
    T& emplace(Args&&... args) {
        has_value_ = false;
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
        char dummy_;
    };
    bool has_value_;
};


template <class T, class EnableDtor = void>
class optional : public optional_base<T> {
  public:
    using optional_base<T>::optional_base;

    ~optional() { destroy(); }

    template <class U = T>
    optional& operator=(U&& value) {
        destroy();
        optional_base<T>::operator=(std::forward<U>(value));
    }

    optional& operator=(nullopt_t) noexcept {
        destroy();
        optional_base<T>::operator=(nullopt);
    }

    void reset() noexcept {
        destroy();
        optional_base<T>::reset();
    }

    template <class... Args>
    T& emplace(Args&&... args) {
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
