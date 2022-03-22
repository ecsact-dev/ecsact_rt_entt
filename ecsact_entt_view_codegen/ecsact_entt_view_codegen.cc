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

		std::string entt_exclude_type_string;
		std::string entt_view_type_string;

		options.out_stream
			<< "\tdecltype(auto) ecsact_entt_view(std::type_identity<struct "
			<< sys_cpp_name << ">, auto& registry) {\n";

		for(auto& [name, cap] : sys.capabilities) {
			auto cpp_name = ecsact::lang_cc::to_cpp_identifier(name, package);
			if(cap.excludes()) {
				entt_exclude_type_string += cpp_name + ", ";
			} else if(cap.can_read()) {
				entt_view_type_string += cpp_name + ", ";
			} else if(cap.includes()) {
				entt_view_type_string += cpp_name + ", ";
			}
		}

		options.out_stream << "\t\treturn registry.view<";

		if(!entt_view_type_string.empty()) {
			// remove trailing ", "
			entt_view_type_string = entt_view_type_string.substr(
				0,
				entt_view_type_string.length() - 2
			);

			options.out_stream << entt_view_type_string;
		}

		options.out_stream << ">(";

		if(!entt_exclude_type_string.empty()) {
			// remove trailing ", "
			entt_exclude_type_string = entt_exclude_type_string.substr(
				0,
				entt_exclude_type_string.length() - 2
			);

			options.out_stream
				<< "::entt::exclude<" << entt_exclude_type_string << ">";
		}

		options.out_stream
			<< ");\n"
			<< "\t}\n\n";
	}

	options.out_stream
		<< "} // namespace "
		<< ecsact::lang_cc::to_cpp_identifier(package.name) << "\n";
}
