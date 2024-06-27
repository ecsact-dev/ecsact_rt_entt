#include "gtest/gtest.h"

#include "entt/entt.hpp"
#include "ecsact/runtime/core.hh"
#include "assoc_test.ecsact.hh"

using namespace assoc_test;

TEST(Assoc, EnttSanityChecks) {
	using namespace entt::literals;

	struct A {
		int32_t n;
	};

	auto reg = entt::registry{};
	auto entity = reg.create();

	auto other_storage_id = entt::id_type{1232131231};

	reg.storage<A>().emplace(entity, A{10});
	ASSERT_TRUE(reg.storage<A>().contains(entity));
	ASSERT_FALSE(reg.storage<A>(other_storage_id).contains(entity));

	reg.storage<A>(other_storage_id).emplace(entity, A{20});
	ASSERT_TRUE(reg.storage<A>().contains(entity));
	ASSERT_TRUE(reg.storage<A>(other_storage_id).contains(entity));

	ASSERT_EQ(reg.storage<A>().get(entity).n, 10);
	ASSERT_EQ(reg.storage<A>(other_storage_id).get(entity).n, 20);
}

TEST(AssocCore, EntityAssoc) {
	auto reg = ecsact::core::registry{"EntityAssoc"};
	auto entity1 = reg.create_entity();
	auto entity2 = reg.create_entity();

	ASSERT_FALSE(reg.has_component<EntityAssoc>(entity1, entity2));
	ASSERT_FALSE(reg.has_component<EntityAssoc>(entity1, entity1));

	ASSERT_EQ(
		reg.add_component(entity1, EntityAssoc{entity2, 10}),
		ECSACT_ADD_OK
	);
	ASSERT_TRUE(reg.has_component<EntityAssoc>(entity1, entity2));
	EXPECT_FALSE(reg.has_component<EntityAssoc>(entity1, entity1));
	ASSERT_EQ(reg.get_component<EntityAssoc>(entity1, entity2).n, 10);

	ASSERT_EQ(
		reg.update_component(entity1, EntityAssoc{entity1, 12}, entity2),
		ECSACT_UPDATE_OK
	);
	EXPECT_TRUE(reg.has_component<EntityAssoc>(entity1, entity1));
	EXPECT_FALSE(reg.has_component<EntityAssoc>(entity1, entity2));

	ASSERT_EQ(
		reg.add_component(entity1, EntityAssoc{entity2, 18}),
		ECSACT_ADD_OK
	);
	ASSERT_TRUE(reg.has_component<EntityAssoc>(entity1, entity1));
	ASSERT_TRUE(reg.has_component<EntityAssoc>(entity1, entity2));
	ASSERT_EQ(reg.get_component<EntityAssoc>(entity1, entity1).n, 12);
	ASSERT_EQ(reg.get_component<EntityAssoc>(entity1, entity2).n, 18);

	reg.remove_component<EntityAssoc>(entity1, entity2);
	ASSERT_FALSE(reg.has_component<EntityAssoc>(entity1, entity2));
	ASSERT_TRUE(reg.has_component<EntityAssoc>(entity1, entity1));
	ASSERT_EQ(reg.get_component<EntityAssoc>(entity1, entity1).n, 12);

	reg.remove_component<EntityAssoc>(entity1, entity1);
	ASSERT_FALSE(reg.has_component<EntityAssoc>(entity1, entity2));
	ASSERT_FALSE(reg.has_component<EntityAssoc>(entity1, entity1));
}

TEST(AssocCore, FieldAssoc) {
	auto reg = ecsact::core::registry{"FieldAssoc"};

	auto entity1 = reg.create_entity();
	auto entity2 = reg.create_entity();

	ASSERT_FALSE(reg.has_component<FieldAssoc>(entity1, 10));
	ASSERT_FALSE(reg.has_component<FieldAssoc>(entity1, 11));

	ASSERT_EQ(reg.add_component(entity1, FieldAssoc{10, 44}), ECSACT_ADD_OK);
	ASSERT_TRUE(reg.has_component<FieldAssoc>(entity1, 10));
	ASSERT_FALSE(reg.has_component<FieldAssoc>(entity1, 11));

	ASSERT_EQ(reg.add_component(entity1, FieldAssoc{11, 44}), ECSACT_ADD_OK);
	ASSERT_TRUE(reg.has_component<FieldAssoc>(entity1, 10));
	ASSERT_TRUE(reg.has_component<FieldAssoc>(entity1, 11));

	ASSERT_EQ(
		reg.update_component(entity1, FieldAssoc{9, 44}, 10),
		ECSACT_UPDATE_OK
	);
	ASSERT_FALSE(reg.has_component<FieldAssoc>(entity1, 10));
	ASSERT_TRUE(reg.has_component<FieldAssoc>(entity1, 9));
	ASSERT_TRUE(reg.has_component<FieldAssoc>(entity1, 11));

	reg.remove_component<FieldAssoc>(entity1, 9);
	ASSERT_FALSE(reg.has_component<FieldAssoc>(entity1, 9));
	ASSERT_TRUE(reg.has_component<FieldAssoc>(entity1, 11));
}
