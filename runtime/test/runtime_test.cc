#include "gtest/gtest.h"

#include <array>
#include <set>
#include <typeindex>
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
	ctx._ctx.generate(ctx.get<ComponentA>());
}

void runtime_test::TestAction::impl(context& ctx) {
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

void runtime_test::SimpleIncrementImportedComp::impl(context& ctx) {
	auto comp = ctx.get<imported::test_pkg::ImportedComponent>();
	comp.num += 1;
	ctx.update(comp);
}

void imported::test_pkg::ImportedSystem::impl(context& ctx) {
	auto comp = ctx.get<SomeLocalComponent>();
	comp.local_num += 1;
	ctx.update(comp);
}

static std::atomic_bool AssocTestAction_ran = false;

void runtime_test::AssocTestAction::impl(context& ctx) {
	AssocTestAction_ran = true;
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

static std::atomic_bool AddAssocTest_ran = false;

void runtime_test::AddAssocTest::impl(context& ctx) {
	AddAssocTest_ran = true;
	auto other_entity = ctx.get<OtherEntityComponent>();

	// Get Target other context from OtherEntityComponent
	auto target_ctx = ctx._ctx.other(other_entity.target);
	target_ctx.add(AddAssocTestComponent{.num = 10});
}

static std::atomic_bool RemoveAssocTest_ran = false;

void runtime_test::RemoveAssocTest::impl(context& ctx) {
	RemoveAssocTest_ran = true;
	auto other_entity = ctx.get<OtherEntityComponent>();

	// Get Target other context from OtherEntityComponent
	auto target_ctx = ctx._ctx.other(other_entity.target);
	target_ctx.remove<RemoveAssocTestComponent>();
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

TEST(Core, TrivialRemoveEvent) {
	auto reg = ecsact::core::registry("TrivialRemoveEvent");
	auto entity = reg.create_entity();

	reg.add_component(entity, runtime_test::TrivialRemoveComponent{});
	reg.add_component<runtime_test::WillRemoveTrivial>(entity);

	auto event_happened = false;
	auto evc = ecsact::core::execution_events_collector<>{};

	evc.set_remove_callback<runtime_test::TrivialRemoveComponent>(
		[&](ecsact_entity_id, const auto&) { event_happened = true; }
	);

	reg.execute_systems(1, evc);

	EXPECT_TRUE(event_happened);
}

TEST(Core, EventCollector) {
	auto reg = ecsact::core::registry{"EventCollector"};
	auto entity = reg.create_entity();

	// Checking if we get the init event for a new component added
	{
		auto event_happened = false;
		auto evc = ecsact::core::execution_events_collector<>{};
		evc.set_init_callback<ComponentA>([&](ecsact_entity_id, const ComponentA&) {
			event_happened = true;
		});

		auto test_comp = ComponentA{};
		test_comp.a = 10;

		auto exec_options = ecsact::core::execution_options{};
		exec_options.add_component(entity, &test_comp);

		auto exec_err = reg.execute_systems(std::array{exec_options}, evc);
		EXPECT_EQ(exec_err, ECSACT_EXEC_SYS_OK);

		EXPECT_TRUE(event_happened) << "Init event did not happen";
		event_happened = false;
	}

	// Checking if we get the update event
	{
		auto event_happened = false;
		auto evc = ecsact::core::execution_events_collector<>{};
		evc.set_update_callback<ComponentA>(
			[&](ecsact_entity_id, const ComponentA&) { event_happened = true; }
		);

		auto test_comp = ComponentA{};
		test_comp.a = 42;

		auto exec_options = ecsact::core::execution_options{};
		exec_options.update_component(entity, &test_comp);

		auto exec_err = reg.execute_systems(std::array{exec_options}, evc);
		EXPECT_EQ(exec_err, ECSACT_EXEC_SYS_OK);

		EXPECT_TRUE(event_happened) << "Update event did not happen";
		event_happened = false;
	}

	// Checking if we get the remove_event
	{
		auto event_happened = false;
		auto evc = ecsact::core::execution_events_collector<>{};
		evc.set_remove_callback<ComponentA>(
			[&](ecsact_entity_id, const ComponentA&) { event_happened = true; }
		);

		auto exec_options = ecsact::core::execution_options{};
		exec_options.remove_component<ComponentA>(entity);

		auto exec_err = reg.execute_systems(std::array{exec_options}, evc);
		EXPECT_EQ(exec_err, ECSACT_EXEC_SYS_OK);

		EXPECT_TRUE(event_happened) << "Remove event did not happen";
		event_happened = false;
	}
}

TEST(Core, ExecuteSystemsErrors) {
	auto reg = ecsact::core::registry("ExecuteSystemsErrors");
	auto options = ecsact::core::execution_options{};
	auto test_action = runtime_test::AssocTestAction{
		.assoc_entity = static_cast<ecsact_entity_id>(4000),
	};
	options.push_action(&test_action);

	auto exec_err = reg.execute_systems(std::array{options});

	EXPECT_EQ(exec_err, ECSACT_EXEC_SYS_ERR_ACTION_ENTITY_INVALID);
}

TEST(Core, ExecuteSystemsAssocActionOk) {
	auto reg = ecsact::core::registry("ExecuteSystemsErrors");
	auto test_entity = reg.create_entity();

	auto options = ecsact::core::execution_options{};
	auto test_action = runtime_test::AssocTestAction{
		.assoc_entity = test_entity,
	};

	options.push_action(&test_action);

	auto exec_err = reg.execute_systems(std::array{options});
	EXPECT_EQ(exec_err, ECSACT_EXEC_SYS_OK);
}

TEST(Core, AddAssocOk) {
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::AssocTestAction::id),
		&runtime_test__AssocTestAction
	);
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::AddAssocTest::id),
		&runtime_test__AddAssocTest
	);

	auto reg = ecsact::core::registry("AddAssocOk");
	auto test_entity1 = reg.create_entity();
	reg.add_component(
		test_entity1,
		runtime_test::ComponentA{
			.a = 42,
		}
	);

	auto test_entity2 = reg.create_entity();
	reg.add_component<runtime_test::AddAssocTestTag>(test_entity2);

	auto options = ecsact::core::execution_options{};
	auto test_action = runtime_test::AssocTestAction{
		.assoc_entity = test_entity2,
	};

	options.push_action(&test_action);
	AddAssocTest_ran = false;
	AssocTestAction_ran = false;
	auto exec_err = reg.execute_systems(std::array{options});
	EXPECT_TRUE(AddAssocTest_ran) << "AddAssocTest Impl Didn't Executed";
	EXPECT_TRUE(AssocTestAction_ran) << "AssocTestAction Impl Didn't Executed";
	EXPECT_EQ(exec_err, ECSACT_EXEC_SYS_OK);

	exec_err = ecsact_execute_systems(reg.id(), 1, nullptr, nullptr);
	EXPECT_EQ(exec_err, ECSACT_EXEC_SYS_OK);
}

TEST(Core, RemoveAssocOk) {
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::AssocTestAction::id),
		&runtime_test__AssocTestAction
	);
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::RemoveAssocTest::id),
		&runtime_test__RemoveAssocTest
	);

	auto reg = ecsact::core::registry("RemoveAssocOk");
	auto test_entity2 = reg.create_entity();
	reg.add_component<runtime_test::RemoveAssocTestTag>(test_entity2);
	reg.add_component(
		test_entity2,
		runtime_test::RemoveAssocTestComponent{
			.num = 42,
		}
	);

	auto test_entity1 = reg.create_entity();
	reg.add_component(
		test_entity1,
		runtime_test::OtherEntityComponent{
			.target = test_entity2,
		}
	);

	auto test_entity3 = reg.create_entity();
	reg.add_component(
		test_entity3,
		runtime_test::OtherEntityComponent{
			.target = test_entity2,
		}
	);

	RemoveAssocTest_ran = false;
	reg.execute_systems();
	EXPECT_TRUE(RemoveAssocTest_ran) << "RemoveAssocTest Impl Didn't Executed";

	ASSERT_FALSE(
		reg.has_component<runtime_test::RemoveAssocTestComponent>(test_entity2)
	);

	reg.execute_systems();

	reg.add_component(
		test_entity2,
		runtime_test::RemoveAssocTestComponent{
			.num = 42,
		}
	);

	reg.execute_systems();

	ASSERT_FALSE(
		reg.has_component<runtime_test::RemoveAssocTestComponent>(test_entity2)
	);
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

			// Santity check - no exception
			ctx.other(ctx.get<runtime_test::Attacking>().target);
		}
	);

	attack_damage_exec_count = 0;
	attack_damage_weakened_exec_count = 0;
	reg.execute_systems();
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
		reg.execute_systems();
		EXPECT_EQ(attack_damage_exec_count, attacker_entities.size());
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

	reg.execute_systems();

	// Sanity check
	ASSERT_TRUE(reg.has_component<ComponentA>(entity));

	auto comp_get = reg.get_component<ComponentA>(entity);

	EXPECT_NE(comp_get.a, comp.a);

	// Simulate what the system should be doing.
	comp.a += 2;
	EXPECT_EQ(comp_get.a, comp.a);
}

TEST(Core, GeneratesCreateEvent) {
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::MakeAnother::id),
		&runtime_test__MakeAnother
	);

	auto reg = ecsact::core::registry("GeneratesCreateEvent");

	auto test_entity = reg.create_entity();
	auto test_action = runtime_test::MakeAnother{};
	reg.add_component(test_entity, runtime_test::ComponentA{});

	auto options = ecsact::core::execution_options{};
	options.push_action(&test_action);

	auto evc = ecsact::core::execution_events_collector<>{};
	auto event_happened = false;
	evc.set_entity_created_callback(
		[&](ecsact_entity_id, ecsact_placeholder_entity_id placeholder) {
			event_happened = true;
			ASSERT_EQ(placeholder, ecsact_generated_entity);
		}
	);

	auto exec_err = reg.execute_systems(std::array{options}, evc);
	EXPECT_EQ(exec_err, ECSACT_EXEC_SYS_OK);
	EXPECT_TRUE(event_happened);
	EXPECT_EQ(2, reg.count_entities());
}

TEST(Core, CreateAndDestroyEntity) {
	auto reg = ecsact::core::registry("CreateAndDestroyEntity");

	runtime_test::EntityTesting component_a{.a = 6};

	struct callback_info {
		ecsact_entity_id             entity_id;
		bool                         entity_created = false;
		bool                         entity_destroyed = false;
		ecsact_placeholder_entity_id placeholder_entity_id;
	};

	auto info = callback_info{};
	auto options = ecsact::core::execution_options{};
	auto evc = ecsact::core::execution_events_collector<>{};

	evc.set_entity_created_callback(
		[&](
			ecsact_entity_id             entity_id,
			ecsact_placeholder_entity_id placeholder_entity_id
		) {
			info.entity_created = true;
			info.entity_id = entity_id;
			info.placeholder_entity_id = placeholder_entity_id;
		}
	);

	evc.set_entity_destroyed_callback([&](ecsact_entity_id entity_id) {
		info.entity_destroyed = true;
		info.entity_id = entity_id;
	});

	options.create_entity(static_cast<ecsact_placeholder_entity_id>(42))
		.add_component(&component_a);
	auto exec_err = reg.execute_systems(std::array{options}, evc);
	EXPECT_EQ(exec_err, ECSACT_EXEC_SYS_OK);

	ASSERT_TRUE(info.entity_created);
	ASSERT_FALSE(info.entity_destroyed);
	ASSERT_EQ(reg.count_entities(), 1);

	EXPECT_EQ(
		info.placeholder_entity_id,
		static_cast<ecsact_placeholder_entity_id>(42)
	);

	auto comp = reg.get_component<runtime_test::EntityTesting>(info.entity_id);

	ASSERT_EQ(comp.a, 6);

	options.clear();
	options.destroy_entity(info.entity_id);

	exec_err = reg.execute_systems(std::array{options}, evc);
	ASSERT_EQ(exec_err, ECSACT_EXEC_SYS_OK);
	ASSERT_EQ(reg.count_entities(), 0);
	ASSERT_TRUE(info.entity_destroyed);
}

TEST(Core, MultiPkgUpdate) {
	using imported::test_pkg::ImportedComponent;
	using imported::test_pkg::SomeLocalComponent;

	ASSERT_TRUE(ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(
			runtime_test::SimpleIncrementImportedComp::id
		),
		&runtime_test__SimpleIncrementImportedComp
	));

	ASSERT_TRUE(ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(imported::test_pkg::ImportedSystem::id
		),
		&imported__test_pkg__ImportedSystem
	));

	auto reg = ecsact::core::registry("MultiPkgUpdate");
	auto test_entity = reg.create_entity();
	reg.add_component(test_entity, ImportedComponent{});
	reg.add_component(test_entity, SomeLocalComponent{});

	for(int i = 0; 10 > i; ++i) {
		auto event_happened = std::set<std::type_index>{};
		auto evc = ecsact::core::execution_events_collector<>{};
		evc.set_update_callback<ImportedComponent>([&](auto entity, auto comp) {
			event_happened.insert(typeid(ImportedComponent));
			EXPECT_EQ(comp.num, i + 1);
		});
		evc.set_update_callback<SomeLocalComponent>([&](auto entity, auto comp) {
			event_happened.insert(typeid(SomeLocalComponent));
			EXPECT_EQ(comp.local_num, i + 1);
		});

		reg.execute_systems(1, evc);
		{
			auto c = reg.get_component<ImportedComponent>(test_entity);
			EXPECT_EQ(c.num, i + 1);
			EXPECT_TRUE(event_happened.contains(typeid(ImportedComponent)));
		}

		{
			auto c = reg.get_component<SomeLocalComponent>(test_entity);
			EXPECT_EQ(c.local_num, i + 1);
			EXPECT_TRUE(event_happened.contains(typeid(SomeLocalComponent)));
		}
	}
}

TEST(Core, NoAction) {
	static bool action_executed = false;

	ASSERT_TRUE(ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::TestAction::id),
		[](ecsact_system_execution_context*) { action_executed = true; }
	));

	auto reg = ecsact::core::registry("Core_NoAction");

	auto test_entity = reg.create_entity();
	reg.add_component(test_entity, runtime_test::ComponentA{});

	reg.execute_systems();

	ASSERT_FALSE(action_executed);

	auto exec_opts = ecsact::core::execution_options{};

	auto exec_err = reg.execute_systems(std::array{exec_opts});
	ASSERT_EQ(exec_err, ECSACT_EXEC_SYS_OK);
	ASSERT_FALSE(action_executed);

	auto evc = ecsact::core::execution_events_collector<>{};

	exec_err = reg.execute_systems(std::array{exec_opts}, evc);
	ASSERT_EQ(exec_err, ECSACT_EXEC_SYS_OK);
	ASSERT_FALSE(action_executed);
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
