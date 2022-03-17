#include <boost/config.hpp>
#include <fstream>
#include <ecsact/parser2/plugin.hh>
#include <ecsact/entt/ecsact_entt_view_codegen.hh>

namespace ecsact {

	class ecsact_entt_view_codegen_plugin : public ecsact::plugin {
	public:
		~ecsact_entt_view_codegen_plugin() override {}

		std::string name() const override { return "ecsact_entt_view"; }

		bool process
			( const ecsact::package&   package
			, std::vector<std::string>  args
			, std::filesystem::path     output_path
			) override
		{
			std::ofstream out_stream(output_path);
			ecsact_entt_view_codegen_options options{.out_stream = out_stream};
			ecsact_entt_view_codegen(package, options);

			return true;
		}
	};

	extern "C" BOOST_SYMBOL_EXPORT ecsact_entt_view_codegen_plugin plugin;
	ecsact_entt_view_codegen_plugin plugin;
}

