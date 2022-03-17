#include "gtest/gtest.h"

#include <type_traits>

#include <ecsact/entt/strict_registry.hh>

struct TestComponentA {

};

struct TestComponentB {

};

struct test_package_meta_info {
	static constexpr auto ecsact_tag = ecsact::package_tag;

	using components = ecsact::mp_list<TestComponentA, TestComponentB>;

	using actions = ecsact::mp_list<>;

	using systems = ecsact::mp_list<>;
};

TEST(strict_registry, default_ctor) {
	ecsact::entt::strict_registry<test_package_meta_info> reg;
}

TEST(strict_registry, move_ctor) {
	ecsact::entt::strict_registry<test_package_meta_info> reg1;
	ecsact::entt::strict_registry<test_package_meta_info> reg2(std::move(reg1));
}

TEST(strict_registry, copy_ctor) {
	ecsact::entt::strict_registry<test_package_meta_info> reg1;
	ecsact::entt::strict_registry<test_package_meta_info> reg2(reg1);
}

TEST(strict_registry, move_assign) {
	ecsact::entt::strict_registry<test_package_meta_info> reg1;
	ecsact::entt::strict_registry<test_package_meta_info> reg2;
	reg2 = std::move(reg1);
}

TEST(strict_registry, size) {
	ecsact::entt::strict_registry<test_package_meta_info> reg;
	EXPECT_EQ(reg.size(), 0);
	EXPECT_EQ((reg.size<TestComponentA>()), 0);
	EXPECT_EQ((reg.size<TestComponentB>()), 0);
}

