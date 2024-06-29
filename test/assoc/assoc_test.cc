#include "gtest/gtest.h"

#include <atomic>
#include "entt/entt.hpp"
#include "ecsact/runtime/core.hh"
#include "ecsact/runtime/dynamic.h"
#include "assoc_test.ecsact.hh"
#include "assoc_test.ecsact.systems.hh"

using namespace assoc_test;

#define SET_SYSTEM_IMPL(SystemName)                        \
	ASSERT_TRUE(ecsact_set_system_execution_impl(            \
		ecsact_id_cast<ecsact_system_like_id>(SystemName::id), \
		&assoc_test__##SystemName                              \
	))
#define CLEAR_SYSTEM_IMPL(SystemName)                      \
	ASSERT_TRUE(ecsact_set_system_execution_impl(            \
		ecsact_id_cast<ecsact_system_like_id>(SystemName::id), \
		nullptr                                                \
	))

static std::atomic_int FieldAssocSystem_exec_count = 0;

auto FieldAssocSystem::impl(context& ctx) -> void {
	FieldAssocSystem_exec_count += 1;
}

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

TEST(AssocCore, FieldAssocExecutionCount) {
	SET_SYSTEM_IMPL(FieldAssocSystem);

	auto reg = ecsact::core::registry{"FieldAssocExecutionCount"};

	auto entity1 = reg.create_entity();
	auto entity2 = reg.create_entity();

	reg.add_component(entity1, FieldAssoc{10, 22});
	reg.add_component(entity1, FieldAssoc{11, 30});
	reg.add_component(entity1, FieldAssoc{16, 48});

	reg.execute_systems();
	EXPECT_EQ(FieldAssocSystem_exec_count, 0);
	FieldAssocSystem_exec_count = 0;

	reg.add_component(entity2, FieldAssoc{10, 55});
	reg.add_component(entity2, A{57});

	reg.execute_systems();
	EXPECT_EQ(FieldAssocSystem_exec_count, 1);
	FieldAssocSystem_exec_count = 0;

	CLEAR_SYSTEM_IMPL(FieldAssocSystem);
}
