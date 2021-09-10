workspace(name = "com_seaube_ecs_idl_entt")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

http_archive(
    name = "com_github_skypjack_entt",
    strip_prefix = "entt-3.8.1",
    url = "https://github.com/skypjack/entt/archive/refs/tags/v3.8.1.tar.gz",
    sha256 = "a2b767f06bca67a73a4d71fb9ebb6ed823bb5146faad3c282b9dbbbdae1aa01b",
)

http_archive(
    name = "bzlws",
    strip_prefix = "bzlws-f929e5380f441f50a77776d34a7df8cacdbdf986",
    url = "https://github.com/zaucy/bzlws/archive/f929e5380f441f50a77776d34a7df8cacdbdf986.zip",
    sha256 = "5bebb821b158b11d81dd25cf031b5b26bae97dbb02025df7d0e41a262b3a030b",
)

load("@bzlws//:repo.bzl", "bzlws_deps")
bzlws_deps()

http_archive(
    name = "com_grail_bazel_toolchain",
    strip_prefix = "bazel-toolchain-76ce37e977a304acf8948eadabb82c516320e286",
    urls = ["https://github.com/grailbio/bazel-toolchain/archive/76ce37e977a304acf8948eadabb82c516320e286.zip"],
    sha256 = "6be060b6b4dfb75e6fbb3e6afa774625aa7b67ae45ae52d56e8377508740d9b5",
)

load("@com_grail_bazel_toolchain//toolchain:deps.bzl", "bazel_toolchain_dependencies")

bazel_toolchain_dependencies()

load("@com_grail_bazel_toolchain//toolchain:rules.bzl", "llvm_toolchain")

llvm_toolchain(
    name = "llvm_toolchain",
    llvm_version = "12.0.0",
)

load("@llvm_toolchain//:toolchains.bzl", "llvm_register_toolchains")

llvm_register_toolchains()

git_repository(
    name = "bazel_compdb",
    remote = "git@github.com:grailbio/bazel-compilation-database.git",
    commit = "9682280a2f7e014e870e2654f1e788345bdf0559",
    shallow_since = "1603518373 -0700",
)

http_archive(
    name = "boost",
    strip_prefix = "boost-7ffa896bfff216a3bfedb2cbac1933f8e31066bc",
    urls = ["https://github.com/bazelboost/boost/archive/7ffa896bfff216a3bfedb2cbac1933f8e31066bc.zip"],
    sha256 = "1c5e5466c17b2918b7c9adb181cad490db3bea2f02b15cd73b11b1506016ab9c",
)

load("@boost//:index.bzl", "boost_http_archives")
boost_http_archives()

http_archive(
    name = "com_google_googletest",
    strip_prefix = "googletest-5c08f92c881b666998a4f7852c3cf9e393bf33a7",
    url = "https://github.com/zaucy/googletest/archive/5c08f92c881b666998a4f7852c3cf9e393bf33a7.zip",
    sha256 = "af1d807468ce8caf4215f9b2d50b9a4eade7d29dd82e02e29edeccd49b1b430a",
)
