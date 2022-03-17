#pragma once

#include <filesystem>
#include <ostream>

#include <ecsact/parser2.hh>

namespace ecsact {

	struct ecsact_entt_view_codegen_options {
		std::ostream& out_stream;
	};

	void ecsact_entt_view_codegen
		( const ecsact::package&             package
		, ecsact_entt_view_codegen_options&  options
		);

}
