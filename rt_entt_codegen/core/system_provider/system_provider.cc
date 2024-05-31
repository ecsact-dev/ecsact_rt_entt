#include "system_provider.hh"

ecsact::rt_entt_codegen::core::provider::system_provider::system_provider(
	system_like_id_variant id
)
	: sys_like_id_variant(id) {
	system_details = ecsact_entt_system_details::from_system_like(
		sys_like_id_variant.get_sys_like_id()
	);

	assert(sys_like_id_variant != system_like_id_variant{});
}

ecsact::rt_entt_codegen::core::provider::system_provider::~system_provider() =
	default;
