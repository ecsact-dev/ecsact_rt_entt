#include <string>
#include <filesystem>
#include <cstdlib>

#include <gtest/gtest.h>
#include <boost/process.hpp>

namespace fs = std::filesystem;

TEST(Build, Success) {
	auto ecsact_cli = std::getenv("ECSACT_CLI");
	auto codegen_plugin_path = std::getenv("CODEGEN_PLUGIN_PATH");
	auto ecsact_runtime_file_path = std::getenv("ECSACT_RUNTIME_FILE_PATH");
	auto ecsact_imported_file_path = std::getenv("ECSACT_IMPORTED_FILE_PATH");
	auto ecsact_recipe_path = std::getenv("ECSACT_RECIPE_PATH");

	ASSERT_NE(ecsact_cli, nullptr);
	ASSERT_NE(codegen_plugin_path, nullptr);
	ASSERT_NE(ecsact_runtime_file_path, nullptr);
	ASSERT_NE(ecsact_imported_file_path, nullptr);
	ASSERT_NE(ecsact_recipe_path, nullptr);

	ASSERT_TRUE(fs::exists(ecsact_cli));
	ASSERT_TRUE(fs::exists(codegen_plugin_path));
	ASSERT_TRUE(fs::exists(ecsact_runtime_file_path));
	ASSERT_TRUE(fs::exists(ecsact_imported_file_path));
	ASSERT_TRUE(fs::exists(ecsact_recipe_path));

	boost::process.spawn(
		ecsact_cli,
		std::string(codegen_plugin_path),
		std::string(ecsact_runtime_file_path),
		std::string(ecsact_imported_file_path),
		std::string(ecsact_recipe_path)
	);
}
