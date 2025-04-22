#include "gtest/gtest.h"

#include <array>
#include "ecsact/runtime/core.hh"
#include "ecsact/runtime/dynamic.h"

#include "assoc.ecsact.hh"
#include "assoc.ecsact.systems.hh"

auto assoc_test::DoDamage::impl(context& ctx) -> void {
	auto target_health = ctx.other().get<Health>();
	target_health.value -= ctx.get<Power>().value;
	ctx.other().update(target_health);
}

TEST(Assoc, AddComponentNoEntity) {
	auto reg = ecsact::core::registry{"assoc_test_add_component"};
	auto attacker = reg.create_entity();

	ASSERT_FALSE(ecsact_entity_exists(reg.id(), ecsact_entity_id{}));

	auto err = reg.add_component(
		attacker,
		assoc_test::Attacker{
			// invalid entity! doesn't exist!
			.target = {},
		}
	);
	EXPECT_EQ(err, ECSACT_ADD_ERR_ENTITY_INVALID);
}
