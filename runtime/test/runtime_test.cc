#include "gtest/gtest.h"

#include <ecsact/runtime/core.h>

#include "runtime/test/runtime_test.ecsact.hh"

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
