#include "state_machine/variant.h"

#include "gtest/gtest.h"

namespace {
using ::state_machine::variant::bad_variant_access;
using ::state_machine::variant::empty;
using ::state_machine::variant::Variant;

struct A {
    A(int a) : value{a} {}
    A(A&&) = default;
    A(const A&) = delete;
    int value;
};

struct B {
    B(int b) : value{b} {}
    B(B&&) = delete;
    B(const B&) = default;
    int value;
};

} // namespace

TEST(variant, ctor_dtor_empty) {
    { Variant<> v{}; }
    { Variant<A, B> v{}; }
}

TEST(variant, ctor_dtor_holding_C) {
    static int ctor_count;
    static int dtor_count;

    class C {
      public:
        C() { ctor_count++; }
        ~C() { dtor_count++; }
    };

    {
        Variant<A, B, C> v{};
        EXPECT_EQ(ctor_count, 0);
        EXPECT_EQ(dtor_count, 0);

        v.emplace<C>();
        EXPECT_EQ(ctor_count, 1);
        EXPECT_EQ(dtor_count, 0);

        EXPECT_TRUE(v.holds<C>());
    }
    EXPECT_EQ(ctor_count, 1);
    EXPECT_EQ(dtor_count, 1);
}

TEST(variant, move_type) {
    static constexpr int expected = 42;

    Variant<A, B> v{};

    ASSERT_TRUE(v.holds<empty>());

    const auto& a = v.set(A{expected});

    EXPECT_EQ(a.value, expected);
    ASSERT_TRUE(v.holds<A>());
    EXPECT_EQ(v.get<A>().value, expected);
}

TEST(variant, copy_type) {
    static constexpr int expected = 42;

    Variant<A, B> v{};

    ASSERT_TRUE(v.holds<empty>());

    const auto& b = v.set(B{expected});

    EXPECT_EQ(b.value, expected);
    ASSERT_TRUE(v.holds<B>());
    EXPECT_EQ(v.get<B>().value, expected);
}

TEST(variant, emplace) {
    static constexpr int expected = 42;

    Variant<A, B> v{};

    ASSERT_FALSE(v.holds<A>());
    const auto& a = v.emplace<A>(expected);

    EXPECT_EQ(a.value, expected);
    ASSERT_TRUE(v.holds<A>());
    EXPECT_EQ(v.get<A>().value, expected);
}

TEST(variant, get_wrong_type) {
    Variant<A, B> v{};

    EXPECT_THROW(v.get<A>(), bad_variant_access);
}

TEST(variant, get_if) {
    Variant<A, B> v{};

    ASSERT_TRUE(v.holds<empty>());
    {
        auto result = v.get_if<A>();
        EXPECT_EQ(result, nullptr);
    }
    {
        auto result = v.get_if<B>();
        EXPECT_EQ(result, nullptr);
    }

    v.set(B{0});
    {
        auto result = v.get_if<A>();
        EXPECT_EQ(result, nullptr);
    }
    {
        auto result = v.get_if<B>();
        EXPECT_NE(result, nullptr);
    }
}

TEST(variant, take_move) {
    static int ctor_count;
    static int dtor_count;

    struct C {
        C() { ctor_count++; }
        C(const C&) { ctor_count++; }
        ~C() { dtor_count++; }
    };

    {
        Variant<A, B, C> v{};

        EXPECT_EQ(ctor_count, 0);
        EXPECT_EQ(dtor_count, 0);

        v.emplace<C>();
        EXPECT_EQ(ctor_count, 1);
        EXPECT_EQ(dtor_count, 0);

        auto c = v.take<C>();

        EXPECT_EQ(ctor_count, 2);
        EXPECT_EQ(dtor_count, 1);
    }

    EXPECT_EQ(ctor_count, 2);
    EXPECT_EQ(dtor_count, 2);
}

TEST(variant, take_copy) {
    static int ctor_count;
    static int dtor_count;

    struct C {
        C() { ctor_count++; }
        C(C&&) { ctor_count++; }
        ~C() { dtor_count++; }
    };

    {
        Variant<A, B, C> v{};

        EXPECT_EQ(ctor_count, 0);
        EXPECT_EQ(dtor_count, 0);

        v.emplace<C>();
        EXPECT_EQ(ctor_count, 1);
        EXPECT_EQ(dtor_count, 0);

        auto c = v.take<C>();

        EXPECT_EQ(ctor_count, 2);
        EXPECT_EQ(dtor_count, 1);
    }

    EXPECT_EQ(ctor_count, 2);
    EXPECT_EQ(dtor_count, 2);
}

TEST(variant, visit_A) {
    static constexpr int expected = 42;
    Variant<A, B> v{};

    v.emplace<A>(expected);

    EXPECT_EQ(v.visit([](auto& a) { return a.value; }), expected);
}

TEST(variant, visit_empty) {
    Variant<A, B> v{};

    EXPECT_THROW(v.visit([](auto& a) { return a.value; }), bad_variant_access);
}
