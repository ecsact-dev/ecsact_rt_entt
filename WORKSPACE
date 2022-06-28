workspace(name = "ecsact_entt")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "ecsact",
    remote = "git@github.com:seaube/ecsact.git",
    commit = "9cb97ce962718b3c92dac91a7ac9dfc61b4bd420",
    shallow_since = "1656433721 -0400",
)

http_archive(
    name = "com_github_skypjack_entt",
    strip_prefix = "entt-3.10.1",
    url = "https://github.com/skypjack/entt/archive/refs/tags/v3.10.1.tar.gz",
    sha256 = "f7031545130bfc06f5fe6b2f8c87dcbd4c1254fab86657e2788b70dfeea57965",
)

http_archive(
    name = "com_grail_bazel_toolchain",
    strip_prefix = "bazel-toolchain-c78a67db025e967707febf04a60d57c2286d21ac",
    url = "https://github.com/yaxum62/bazel-toolchain/archive/c78a67db025e967707febf04a60d57c2286d21ac.tar.gz",
    sha256 = "7fd9b2efd8d9d62a8f5263e917d8b601f4e1fd44affe0c894420b2ee636f8454",
)

load("@com_grail_bazel_toolchain//toolchain:deps.bzl", "bazel_toolchain_dependencies")

bazel_toolchain_dependencies()

load("@com_grail_bazel_toolchain//toolchain:rules.bzl", "llvm_toolchain")

llvm_toolchain(
    name = "llvm_toolchain",
    llvm_version = "13.0.0",
    cxx_standard = "c++20",
)

load("@llvm_toolchain//:toolchains.bzl", "llvm_register_toolchains")

llvm_register_toolchains()

http_archive(
    name = "boost",
    strip_prefix = "boost-563e8e0de4eac4b48a02d296581dc2454127608e",
    urls = ["https://github.com/bazelboost/boost/archive/563e8e0de4eac4b48a02d296581dc2454127608e.zip"],
    sha256 = "c41441a6e9f8038ad626e9f937ddc3675ab896b6c3512eefc6840037b3816d03",
)

load("@boost//:index.bzl", "boost_http_archives")
boost_http_archives()

http_archive(
    name = "com_google_googletest",
    strip_prefix = "googletest-5c08f92c881b666998a4f7852c3cf9e393bf33a7",
    url = "https://github.com/zaucy/googletest/archive/5c08f92c881b666998a4f7852c3cf9e393bf33a7.zip",
    sha256 = "af1d807468ce8caf4215f9b2d50b9a4eade7d29dd82e02e29edeccd49b1b430a",
)

_nlohmann_json_build_file = """
load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "json",
    visibility = ["//visibility:public"],
    includes = ["include"],
    hdrs = glob(["include/**/*.hpp"]),
    strip_include_prefix = "include",
)
"""

http_archive(
    name = "nlohmann_json",
    url = "https://github.com/nlohmann/json/releases/download/v3.10.4/include.zip",
    sha256 = "62c585468054e2d8e7c2759c0d990fd339d13be988577699366fe195162d16cb",
    build_file_content = _nlohmann_json_build_file,
)

http_archive(
    name = "com_google_protobuf",
    strip_prefix = "protobuf-3.18.0",
    urls = ["https://github.com/protocolbuffers/protobuf/releases/download/v3.18.0/protobuf-cpp-3.18.0.tar.gz"],
    sha256 = "7308590dbb95e77066b99c5674eed855c8257e70658d2af586f4a81ff0eea2b1",
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
protobuf_deps()
