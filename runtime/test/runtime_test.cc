#include "gtest/gtest.h"

#include "ecsact/runtime/core.h"
#include "ecsact/runtime/dynamic.h"

#include "runtime_test.ecsact.hh"
#include "runtime_test.ecsact.systems.hh"

#ifndef ECSACT_ENTT_TEST_DYNAMIC_SYSTEM_IMPL
void runtime_test::SimpleSystem(SimpleSystem::context& ctx) {
	auto comp = ctx.get<ComponentA>();
	comp.a += 1;
	ctx.update(comp);
}
#endif

TEST(Core, CreateRegistry) {
	auto reg_id = ecsact_create_registry("CreateRegistry");
	EXPECT_NE(reg_id, ecsact_invalid_registry_id);
}

TEST(Core, DestroyRegistry) {
	auto reg_id = ecsact_create_registry("DestroyRegistry");
	ecsact_destroy_registry(reg_id);
}

TEST(Core, ClearRegistry) {
	auto reg_id = ecsact_create_registry("ClearRegistry");
	[[maybe_unused]]
	auto entity = ecsact_create_entity(reg_id);
	auto entity_count = ecsact_count_entities(reg_id);
	EXPECT_EQ(entity_count, 1);
	ecsact_clear_registry(reg_id);
	entity_count = ecsact_count_entities(reg_id);
	EXPECT_EQ(entity_count, 0);
}

TEST(Core, CreateEntity) {
	auto reg_id = ecsact_create_registry("CreateEntity");
	auto entity = ecsact_create_entity(reg_id);
	EXPECT_NE(entity, ecsact_invalid_entity_id);
}

TEST(Core, EnsureEntity) {
	auto reg_id = ecsact_create_registry("EnsureEntity");
	auto entity = static_cast<ecsact_entity_id>(4);
	EXPECT_FALSE(ecsact_entity_exists(reg_id, entity));
	ecsact_ensure_entity(reg_id, entity);
	EXPECT_TRUE(ecsact_entity_exists(reg_id, entity));
	ecsact_ensure_entity(reg_id, entity);
	EXPECT_TRUE(ecsact_entity_exists(reg_id, entity));
}

TEST(Core, DestroyEntity) {
	auto reg_id = ecsact_create_registry("DestroyEntity");
	auto entity = ecsact_create_entity(reg_id);
	ecsact_destroy_entity(reg_id, entity);
	EXPECT_FALSE(ecsact_entity_exists(reg_id, entity));
}

TEST(Core, CountEntities) {
	auto reg_id = ecsact_create_registry("CountEntities");
	[[maybe_unused]]
	auto entity = ecsact_create_entity(reg_id);
	EXPECT_EQ(ecsact_count_entities(reg_id), 1);
}

TEST(Core, GetEntities) {
	const int test_count = 20;

	auto reg_id = ecsact_create_registry("GetEntities");
	for(int i=0; test_count > i; ++i) {
		ecsact_create_entity(reg_id);
	}

	int entities_count = 0;
	std::vector<ecsact_entity_id> entities;
	entities.resize(test_count);
	for(auto& ent : entities) ent = ecsact_invalid_entity_id;

	ecsact_get_entities(reg_id, test_count, entities.data(), &entities_count);
	ASSERT_EQ(entities_count, test_count);

	for(auto& ent : entities) EXPECT_NE(ent, ecsact_invalid_entity_id);
}

TEST(Core, AddComponent) {
	auto reg_id = ecsact_create_registry("AddComponent");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);

	ecsact_add_component(reg_id, entity, comp_id, &comp);

	EXPECT_TRUE(ecsact_has_component(reg_id, entity, comp_id));
}

TEST(Core, HasComponent) {
	auto reg_id = ecsact_create_registry("HasComponent");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);

	EXPECT_FALSE(ecsact_has_component(reg_id, entity, comp_id));
	ecsact_add_component(reg_id, entity, comp_id, &comp);
	EXPECT_TRUE(ecsact_has_component(reg_id, entity, comp_id));
}

TEST(Core, GetComponent) {
	auto reg_id = ecsact_create_registry("GetComponent");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);
	ecsact_add_component(reg_id, entity, comp_id, &comp);

	auto comp_get = static_cast<const runtime_test::ComponentA*>(
		ecsact_get_component(reg_id, entity, comp_id)
	);

	EXPECT_EQ(*comp_get, comp);
}

TEST(Core, UpdateComponent) {
	auto reg_id = ecsact_create_registry("UpdateComponent");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	runtime_test::ComponentA upped_comp{.a = 43};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);
	ecsact_add_component(reg_id, entity, comp_id, &comp);
	ecsact_update_component(reg_id, entity, comp_id, &upped_comp);

	auto comp_get = static_cast<const runtime_test::ComponentA*>(
		ecsact_get_component(reg_id, entity, comp_id)
	);

	EXPECT_EQ(*comp_get, upped_comp);
}


TEST(Core, RemoveComponent) {
	auto reg_id = ecsact_create_registry("RemoveComponent");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);
	ecsact_add_component(reg_id, entity, comp_id, &comp);
	EXPECT_TRUE(ecsact_has_component(reg_id, entity, comp_id));
	ecsact_remove_component(reg_id, entity, comp_id);
	EXPECT_FALSE(ecsact_has_component(reg_id, entity, comp_id));
}

static void dynamic_impl(ecsact_system_execution_context* ctx) {
	using runtime_test::ComponentA;

	auto comp_id = static_cast<ecsact_component_id>(ComponentA::id);
	ComponentA comp;
	ecsact_system_execution_context_get(ctx, comp_id, &comp);
	comp.a += 2;
	ecsact_system_execution_context_update(ctx, comp_id, &comp);
}

TEST(Core, DynamicSystemImpl) {
	auto reg_id = ecsact_create_registry("DynamicSystemImpl");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);
	ecsact_add_component(reg_id, entity, comp_id, &comp);

	auto comp_get = static_cast<const runtime_test::ComponentA*>(
		ecsact_get_component(reg_id, entity, comp_id)
	);

	// Sanity check
	ASSERT_EQ(*comp_get, comp);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::SimpleSystem::id),
		&dynamic_impl
	);
	ecsact_execute_systems(reg_id, 1, nullptr, nullptr);

	comp_get = static_cast<const runtime_test::ComponentA*>(
		ecsact_get_component(reg_id, entity, comp_id)
	);
	
	EXPECT_NE(*comp_get, comp);

	// Simulate what the system should be doing.
	comp.a += 2;
	EXPECT_EQ(*comp_get, comp);
}

TEST(Core, StaticSystemImpl) {
	auto reg_id = ecsact_create_registry("StaticSystemImpl");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);
	ecsact_add_component(reg_id, entity, comp_id, &comp);

	auto comp_get = static_cast<const runtime_test::ComponentA*>(
		ecsact_get_component(reg_id, entity, comp_id)
	);

	// Sanity check
	ASSERT_EQ(comp_get->a, comp.a);

	// Clear any system impls that may already be set so we can use the static one
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::SimpleSystem::id),
		nullptr
	);

	ecsact_execute_systems(reg_id, 1, nullptr, nullptr);

	comp_get = static_cast<const runtime_test::ComponentA*>(
		ecsact_get_component(reg_id, entity, comp_id)
	);
	
	EXPECT_NE(comp_get->a, comp.a);

	// Simulate what the system should be doing.
	comp.a += 1;
	EXPECT_EQ(comp_get->a, comp.a);
}
