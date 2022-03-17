#include "ecsact_entt_view_codegen.hh"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <ecsact/lang-support/lang-cc.hh>
#include "generator/cpp_systems/util/cpp_systems_util.hh"

void ecsact::ecsact_entt_view_codegen
	( const ecsact::package&             package
	, ecsact_entt_view_codegen_options&  options
	)
{
	options.out_stream
		<< "// GENERATED FILE - DO NOT EDIT\n"
		<< "#pragma once\n\n";

	options.out_stream
		<< "#include <type_traits>\n"
		<< "#include <entt/entt.hpp>\n"
		<< "#include \"" << package.name << ".hh\"\n"
		<< "#include \"" << package.name << ".systems.hh\"\n"
		<< "\n";

	options.out_stream
		<< "namespace "
		<< ecsact::lang_cc::to_cpp_identifier(package.name)
		<< "{\n";

	for(const ecsact::system_like& sys : package.all_system_like) {
		auto sys_cpp_name = ecsact::lang_cc::to_cpp_identifier(
			ecsact::cpp_systems_util::get_system_full_name(sys)
		);

		options.out_stream
			<< "\tauto ecsact_entt_view(std::type_identity<struct "
			<< sys_cpp_name << ">, auto& registry) {\n"
			<< "\t\treturn entt::basic_view{\n";

		for(auto& [name, cap] : sys.capabilities) {
			auto cpp_name = ecsact::lang_cc::to_cpp_identifier(name, package);
			if(cap.excludes()) {
				options.out_stream
					<< "\t\t\t// entt::exclude<" << cpp_name << ">,\n";
			} else if(cap.can_read()) {
				options.out_stream
					<< "\t\t\tregistry.storage<" << cpp_name << ">(),\n";
			} else if(cap.includes()) {
				options.out_stream
					<< "\t\t\tregistry.storage<" << cpp_name << ">(),\n";
			}
		}

		options.out_stream
			<< "\t\t};\n"
			<< "\t}\n\n";
	}

	options.out_stream
		<< "} // namespace "
		<< ecsact::lang_cc::to_cpp_identifier(package.name) << "\n";
}
