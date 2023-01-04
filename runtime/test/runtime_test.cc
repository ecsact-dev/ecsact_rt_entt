#include "gtest/gtest.h"

#include <array>
#include <unordered_set>
#include <version>
#include <ranges>
#include "ecsact/runtime/core.hh"
#include "ecsact/runtime/dynamic.h"

#include "runtime_test.ecsact.hh"
#include "runtime_test.ecsact.systems.hh"

using runtime_test::ComponentA;
using runtime_test::OtherEntityComponent;

void runtime_test::SimpleSystem::impl(context& ctx) {
	auto comp = ctx.get<ComponentA>();
	comp.a += 2;
	ctx.update(comp);
}

void runtime_test::OtherEntitySystem::impl(context& ctx) {
	auto comp = ctx.get<OtherEntityComponent>();
	auto other = ctx._ctx.other(comp.target);
	auto other_comp = other.get<ComponentA>();

	comp.num += -other_comp.a;

	ctx.update(comp);
	other.update(other_comp);
}

void runtime_test::MakeAnother::impl(context& ctx) {
}

void runtime_test::AlwaysRemove::impl(context& ctx) {
	// This trivial remove should not even be required:
	// SEE: https://github.com/ecsact-dev/ecsact_lang_cpp/issues/80
	std::cerr << "AlwaysRemove impl called (SHOULD NOT HAPPEN)\n";
	std::cerr.flush();
	std::abort();
}

void runtime_test::TrivialRemove::impl(context& ctx) {
	// This trivial remove should not even be required:
	// SEE: https://github.com/ecsact-dev/ecsact_lang_cpp/issues/80
	std::cerr << "TriviaLRemove impl called (SHOULD NOT HAPPEN)\n";
	std::cerr.flush();
	std::abort();
}

void runtime_test::AssocTestAction::impl(context& ctx) {
	ctx.add(OtherEntityComponent{
		.num = 42,
		.target = ctx.action().assoc_entity,
	});
}

void runtime_test::AttackDamage::impl(context& ctx) {
	// auto attacking = ctx.get<Attacking>();
	// auto target_ctx = ctx._ctx.other(attacking.target);
	// auto target_health = target_ctx.get<Health>();
	// target_health.value -= 1.f;
	// target_ctx.update(target_health);
}

void runtime_test::AttackDamageWeakened::impl(context& ctx) {
	// auto attacking = ctx.get<Attacking>();
	// auto target_ctx = ctx._ctx.other(attacking.target);
	// auto target_health = target_ctx.get<Health>();
	// auto target_weakened = target_ctx.get<Weakened>();
	// target_health.value -= 1.f * target_weakened.value;
	// target_ctx.update(target_health);
}

TEST(Core, CreateRegistry) {
	auto reg_id = ecsact_create_registry("CreateRegistry");
	EXPECT_NE(reg_id, ecsact_invalid_registry_id);
}

TEST(Core, DestroyRegistry) {
	auto reg_id = ecsact_create_registry("DestroyRegistry");
	ecsact_destroy_registry(reg_id);
}

TEST(Core, ClearRegistry) {
	auto                  reg_id = ecsact_create_registry("ClearRegistry");
	[[maybe_unused]] auto entity = ecsact_create_entity(reg_id);
	auto                  entity_count = ecsact_count_entities(reg_id);
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
	auto                  reg_id = ecsact_create_registry("CountEntities");
	[[maybe_unused]] auto entity = ecsact_create_entity(reg_id);
	EXPECT_EQ(ecsact_count_entities(reg_id), 1);
}

TEST(Core, GetEntities) {
	const int test_count = 20;

	auto reg_id = ecsact_create_registry("GetEntities");
	for(int i = 0; test_count > i; ++i) {
		ecsact_create_entity(reg_id);
	}

	int                           entities_count = 0;
	std::vector<ecsact_entity_id> entities;
	entities.resize(test_count);
	for(auto& ent : entities) {
		ent = ecsact_invalid_entity_id;
	}

	ecsact_get_entities(reg_id, test_count, entities.data(), &entities_count);
	ASSERT_EQ(entities_count, test_count);

	for(auto& ent : entities) {
		EXPECT_NE(ent, ecsact_invalid_entity_id);
	}
}

TEST(Core, AddComponent) {
	auto reg_id = ecsact_create_registry("AddComponent");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);

	auto add_err = ecsact_add_component(reg_id, entity, comp_id, &comp);
	EXPECT_EQ(add_err, ECSACT_ADD_OK);

	EXPECT_TRUE(ecsact_has_component(reg_id, entity, comp_id));
}

TEST(Core, AddComponentError) {
	auto             reg = ecsact::core::registry("AddComponentError");
	auto             entity = reg.create_entity();
	ecsact_entity_id invalid_entity =
		static_cast<ecsact_entity_id>((int)entity + 1);

	OtherEntityComponent comp{.num = 42, .target = invalid_entity};

	auto add_err = reg.add_component(entity, comp);
	// We tried to add a component with an invalid entity ID. We should get an
	// invalid entity error.
	EXPECT_EQ(add_err, ECSACT_ADD_ERR_ENTITY_INVALID);

	// When we receive an error when adding a component our component will not be
	// added to the registry.
	EXPECT_FALSE(reg.has_component<OtherEntityComponent>(entity));
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

TEST(Core, UpdateComponentError) {
	auto             reg = ecsact::core::registry("UpdateComponentError");
	auto             entity = reg.create_entity();
	auto             other_entity = reg.create_entity();
	ecsact_entity_id invalid_entity =
		static_cast<ecsact_entity_id>((int)other_entity + 1);

	OtherEntityComponent comp{.num = 42, .target = other_entity};

	auto add_err = reg.add_component(entity, comp);
	ASSERT_EQ(add_err, ECSACT_ADD_OK);

	comp.num = 43;
	comp.target = invalid_entity;
	auto update_err = reg.update_component(entity, comp);

	// We tried to update the component with an invalid entity ID. We should get
	// an invalid entity error.
	EXPECT_EQ(update_err, ECSACT_UPDATE_ERR_ENTITY_INVALID);
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

	auto       comp_id = static_cast<ecsact_component_like_id>(ComponentA::id);
	ComponentA comp;
	ecsact_system_execution_context_get(ctx, comp_id, &comp);
	comp.a += 2;
	ecsact_system_execution_context_update(ctx, comp_id, &comp);
}

TEST(Core, TrivialRemoveEvent) {
	auto reg_id = ecsact_create_registry("TrivialRemoveEvent");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::TrivialRemoveComponent comp{};
	ecsact_add_component(
		reg_id,
		entity,
		runtime_test::TrivialRemoveComponent::id,
		&comp
	);

	ecsact_add_component(
		reg_id,
		entity,
		runtime_test::WillRemoveTrivial::id,
		nullptr
	);

	static bool                       event_happened = false;
	ecsact_execution_events_collector ev_collector{};
	ev_collector.remove_callback = //
		[](
			ecsact_event        event,
			ecsact_entity_id    entity_id,
			ecsact_component_id component_id,
			const void*         component_data,
			void*               callback_user_data
		) {
			event_happened = true;

			EXPECT_EQ(component_id, runtime_test::TrivialRemoveComponent::id);
		};

	ecsact_execute_systems(reg_id, 1, nullptr, &ev_collector);

	EXPECT_TRUE(event_happened);
}

TEST(Core, EventCollector) {
	auto reg = ecsact::core::registry{"EventCollector"};
	auto entity = reg.create_entity();

	// Test if we receive an init, update, and remove event

	static auto event_happened = false;

	auto callback = //
		[](
			ecsact_event        event,
			ecsact_entity_id    entity_id,
			ecsact_component_id component_id,
			const void*         component_data,
			void*               callback_user_data
		) { event_happened = true; };

	// Checking if we get the init event for a new component added
	{
		auto evc = ecsact_execution_events_collector{};
		evc.init_callback = callback;

		auto test_comp = ComponentA{};
		test_comp.a = 10;

		auto add_component_entities = std::array{entity};
		auto add_components = std::array{
			ecsact_component{
				.component_id = ComponentA::id,
				.component_data = &test_comp,
			},
		};

		auto exec_options = ecsact_execution_options{};
		exec_options.add_components_length = add_components.size();
		exec_options.add_components_entities = add_component_entities.data();
		exec_options.add_components = add_components.data();

		ecsact_execute_systems(reg.id(), 1, &exec_options, &evc);

		EXPECT_TRUE(event_happened) << "Init event did not happen";
		event_happened = false;
	}

	// Checking if we get the update event
	{
		auto evc = ecsact_execution_events_collector{};
		evc.update_callback = callback;

		auto test_comp = ComponentA{};
		test_comp.a = 42;

		auto update_component_entities = std::array{entity};
		auto update_components = std::array{
			ecsact_component{
				.component_id = ComponentA::id,
				.component_data = &test_comp,
			},
		};

		auto exec_options = ecsact_execution_options{};
		exec_options.update_components_length = update_components.size();
		exec_options.update_components_entities = update_component_entities.data();
		exec_options.update_components = update_components.data();

		ecsact_execute_systems(reg.id(), 1, &exec_options, &evc);

		EXPECT_TRUE(event_happened) << "Update event did not happen";
		event_happened = false;
	}

	// Checking if we get the remove_event
	{
		auto evc = ecsact_execution_events_collector{};
		evc.remove_callback = callback;

		auto remove_component_entities = std::array{entity};
		auto remove_components = std::array{ComponentA::id};

		auto exec_options = ecsact_execution_options{};
		exec_options.remove_components_length = remove_components.size();
		exec_options.remove_components_entities = remove_component_entities.data();
		exec_options.remove_components = remove_components.data();

		ecsact_execute_systems(reg.id(), 1, &exec_options, &evc);

		EXPECT_TRUE(event_happened) << "Remove event did not happen";
		event_happened = false;
	}
}

TEST(Core, DynamicSystemImpl) {
	auto reg = ecsact::core::registry("DynamicSystemImpl");
	auto entity = reg.create_entity();
	auto other_entity = reg.create_entity();

	ComponentA comp{.a = 42};
	reg.add_component(entity, comp);
	reg.add_component(other_entity, comp);

	OtherEntityComponent other_comp{.num = 3, .target = other_entity};
	ASSERT_EQ(reg.add_component(entity, other_comp), ECSACT_ADD_OK);

	// Sanity check
	ASSERT_TRUE(reg.has_component<ComponentA>(entity));
	ASSERT_EQ(reg.get_component<ComponentA>(entity), comp);
	ASSERT_TRUE(reg.has_component<ComponentA>(other_entity));
	ASSERT_EQ(reg.get_component<ComponentA>(other_entity), comp);
	ASSERT_TRUE(reg.has_component<OtherEntityComponent>(entity));
	ASSERT_EQ(reg.get_component<OtherEntityComponent>(entity), other_comp);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::SimpleSystem::id),
		&runtime_test__SimpleSystem
	);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::OtherEntitySystem::id),
		&runtime_test__OtherEntitySystem
	);

	ecsact_execute_systems(reg.id(), 1, nullptr, nullptr);

	// Sanity check
	ASSERT_TRUE(reg.has_component<ComponentA>(entity));

	auto comp_get = reg.get_component<ComponentA>(entity);

	EXPECT_NE(comp_get.a, comp.a);

	// Simulate what the system should be doing.
	comp.a += 2;
	EXPECT_EQ(comp_get.a, comp.a);
}

TEST(Core, ExecuteSystemsErrors) {
	auto reg = ecsact::core::registry("ExecuteSystemsErrors");
	auto comp = OtherEntityComponent{
		.num = 42,
		.target = static_cast<ecsact_entity_id>(4000),
	};
	auto options = ecsact_execution_options{};
	auto test_action = runtime_test::AssocTestAction{};
	auto test_action_c = ecsact_action{
		.action_id = runtime_test::AssocTestAction::id,
		.action_data = &test_action,
	};

	options.actions_length = 1;
	options.actions = &test_action_c;
	auto exec_err = ecsact_execute_systems(reg.id(), 1, &options, nullptr);

	EXPECT_EQ(exec_err, ECSACT_EXEC_SYS_ERR_ACTION_ENTITY_INVALID);
}

TEST(Core, AssociationEntityCorrectness) {
	using runtime_test::AttackDamage;
	using runtime_test::AttackDamageWeakened;

	static auto reg = ecsact::core::registry("AssociationEntityCorrectness");
	static auto attacker_entities = std::unordered_set{
		reg.create_entity(),
		reg.create_entity(),
		reg.create_entity(),
	};
	static auto weakened_target_entities = std::unordered_set{
		reg.create_entity(),
		reg.create_entity(),
	};
	static auto target_entities = std::unordered_set{
		reg.create_entity(),
	};

	static auto all_target_entities = []() {
		std::unordered_set all_target_entities(
			weakened_target_entities.begin(),
			weakened_target_entities.end()
		);
		all_target_entities.insert(target_entities.begin(), target_entities.end());
		return all_target_entities;
	}();

	for(auto target : all_target_entities) {
		ASSERT_EQ(
			ECSACT_ADD_OK,
			reg.add_component(target, runtime_test::Health{0.5f})
		);
	}

	for(auto target : weakened_target_entities) {
		ASSERT_EQ(
			ECSACT_ADD_OK,
			reg.add_component(target, runtime_test::Weakened{0.5f})
		);
	}

	{
		assert(attacker_entities.size() == all_target_entities.size());
		auto attacker_itr = attacker_entities.begin();
		auto target_itr = all_target_entities.begin();
		for(; attacker_itr != attacker_entities.end();
				++attacker_itr, ++target_itr) {
			ASSERT_EQ(
				ECSACT_ADD_OK,
				reg.add_component(*attacker_itr, runtime_test::Attacking{*target_itr})
			);
		}
	}

	auto print_attack_targets = [&] {
		for(auto attacker : attacker_entities) {
			auto attacking = reg.get_component<runtime_test::Attacking>(attacker);
			std::cout << (int)attacker << " is attacking " << (int)attacking.target
								<< "\n";
		}
	};

	print_attack_targets();

	static std::atomic_int attack_damage_exec_count = 0;
	static std::atomic_int attack_damage_weakened_exec_count = 0;

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(AttackDamageWeakened::id),
		[](ecsact_system_execution_context* cctx) {
			++attack_damage_weakened_exec_count;
			ecsact::execution_context ctx{cctx};
			ASSERT_TRUE(attacker_entities.contains(ctx.entity()));
			auto target_ctx = ctx.other(ctx.get<runtime_test::Attacking>().target);
			ASSERT_TRUE(weakened_target_entities.contains(target_ctx.entity()));
		}
	);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(AttackDamage::id),
		[](ecsact_system_execution_context* cctx) {
			++attack_damage_exec_count;
			ecsact::execution_context ctx{cctx};
			ASSERT_TRUE(attacker_entities.contains(ctx.entity()));
			auto target_ctx = ctx.other(ctx.get<runtime_test::Attacking>().target);
		}
	);

	attack_damage_exec_count = 0;
	attack_damage_weakened_exec_count = 0;
	ecsact_execute_systems(reg.id(), 1, nullptr, nullptr);
	EXPECT_EQ(attack_damage_exec_count, all_target_entities.size());
	EXPECT_EQ(attack_damage_weakened_exec_count, weakened_target_entities.size());

	for(auto target_entity : target_entities) {
		for(auto attack_entity : attacker_entities) {
			reg.update_component(
				attack_entity,
				runtime_test::Attacking{target_entity}
			);
		}

		attack_damage_exec_count = 0;
		attack_damage_weakened_exec_count = 0;
		ecsact_execute_systems(reg.id(), 1, nullptr, nullptr);
		EXPECT_EQ(attack_damage_exec_count, attacker_entities.size());
	}
}

#ifdef ECSACT_ENTT_TEST_STATIC_SYSTEM_IMPL
TEST(Core, StaticSystemImpl) {
	auto reg_id = ecsact_create_registry("StaticSystemImpl");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);
	ecsact_add_component(reg_id, entity, comp_id, &comp);

	auto comp_get = static_cast<const runtime_test::ComponentA*>(
		ecsact_get_component(reg_id, entity, runtime_test::ComponentA::id)
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
	comp.a += 2;
	EXPECT_EQ(comp_get->a, comp.a);
}
#endif // ECSACT_ENTT_TEST_STATIC_SYSTEM_IMPL
