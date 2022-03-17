#include <fstream>
#include <ecsact/parser2.hh>
#include <ecsact/generator-cli.hh>

#include "ecsact_entt_view_codegen.hh"

int main(int argc, char* argv[]) {
	ecsact::parse_results results{};
	ecsact::generator_cli::cli_parse(argc, argv, results);

	for(auto& pkg : results.packages) {
		auto out_file_path =
			pkg.source_file_path.replace_extension(".entt_view.hh");
		std::ofstream out_stream(out_file_path);
		ecsact::ecsact_entt_view_codegen_options options{.out_stream = out_stream};
		ecsact::ecsact_entt_view_codegen(pkg, options);
	}
	
	return 0;
}
