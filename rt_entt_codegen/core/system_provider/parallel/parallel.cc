#include "parallel.hh"

#include "ecsact/cpp_codegen_plugin_util.hh"

using namespace ecsact::rt_entt_codegen::core;

auto provider::parallel::entity_iteration(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::function<void()>                             iter_func
) -> handle_exclusive_provide {
	using ecsact::cpp_codegen_plugin_util::block;

	block(
		ctx,
		"std::for_each(std::execution::par_unseq, view.begin(), "
		"view.end(), [&](auto entity)",
		[&] { iter_func(); }
	);
	ctx.write(");\n");
	return HANDLED;
}
