#include "gtest/gtest.h"

#include <type_traits>

#include <ecs-idl-entt/strict_registry.hh>

struct TestComponentA {

};

struct TestComponentB {

};

struct test_package_meta_info {
  static constexpr auto ecs_idl_tag = ecs_idl::package_meta_info_tag;

  using all_components = ecs_idl::mp_list<TestComponentA, TestComponentB>;

  template<typename T>
  static constexpr bool is_component_v =
    std::is_same_v<T, TestComponentA> ||
    std::is_same_v<T, TestComponentB>;
};

TEST(strict_registry, default_ctor) {
  ecs_idl::entt::strict_registry<test_package_meta_info> reg;
}

TEST(strict_registry, move_ctor) {
  ecs_idl::entt::strict_registry<test_package_meta_info> reg1;
  ecs_idl::entt::strict_registry<test_package_meta_info> reg2(std::move(reg1));
}

TEST(strict_registry, copy_ctor) {
  ecs_idl::entt::strict_registry<test_package_meta_info> reg1;
  ecs_idl::entt::strict_registry<test_package_meta_info> reg2(reg1);
}

TEST(strict_registry, move_assign) {
  ecs_idl::entt::strict_registry<test_package_meta_info> reg1;
  ecs_idl::entt::strict_registry<test_package_meta_info> reg2;
  reg2 = std::move(reg1);
}

TEST(strict_registry, size) {
  ecs_idl::entt::strict_registry<test_package_meta_info> reg;
  EXPECT_EQ(reg.size(), 0);
  EXPECT_EQ(reg.size<TestComponentA>(), 0);
  EXPECT_EQ(reg.size<TestComponentB>(), 0);
}

