#include "gtest/gtest.h"

#include "ecsact/runtime/core.hh"
#include "ecsact/runtime/dynamic.h"

#include "assoc_test.ecsact.hh"
#include "assoc_test.ecsact.systems.hh"

using namespace assoc_test;

TEST(AssocCore, EntityAssoc) {
	auto reg = ecsact::core::registry{"EntityAssoc"};
	auto entity1 = reg.create_entity();
	auto entity2 = reg.create_entity();

	ASSERT_FALSE(reg.has_component<EntityAssoc>(entity1, entity2));
	ASSERT_FALSE(reg.has_component<EntityAssoc>(entity1, entity1));

	reg.add_component(entity1, EntityAssoc{entity2, 10});
	ASSERT_TRUE(reg.has_component<EntityAssoc>(entity1, entity2));
	EXPECT_FALSE(reg.has_component<EntityAssoc>(entity1, entity1));
	ASSERT_EQ(reg.get_component<EntityAssoc>(entity1, entity1).n, 10);

	reg.update_component(entity1, EntityAssoc{entity1, 12}, entity2);
	ASSERT_TRUE(reg.has_component<EntityAssoc>(entity1, entity1));
	ASSERT_FALSE(reg.has_component<EntityAssoc>(entity1, entity2));

	reg.add_component(entity1, EntityAssoc{entity2, 18});
	ASSERT_TRUE(reg.has_component<EntityAssoc>(entity1, entity1));
	ASSERT_TRUE(reg.has_component<EntityAssoc>(entity1, entity2));
	ASSERT_EQ(reg.get_component<EntityAssoc>(entity1, entity1).n, 12);
	ASSERT_EQ(reg.get_component<EntityAssoc>(entity1, entity2).n, 18);

	reg.remove_component<EntityAssoc>(entity1, entity2);
	ASSERT_FALSE(reg.has_component<EntityAssoc>(entity1, entity2));
	ASSERT_TRUE(reg.has_component<EntityAssoc>(entity1, entity1));
	ASSERT_EQ(reg.get_component<EntityAssoc>(entity1, entity1).n, 12);

	reg.remove_component<EntityAssoc>(entity1, entity2);
	ASSERT_FALSE(reg.has_component<EntityAssoc>(entity1, entity2));
	ASSERT_FALSE(reg.has_component<EntityAssoc>(entity1, entity1));
}
