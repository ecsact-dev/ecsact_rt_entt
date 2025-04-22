load("@ecsact_rt_entt//bazel:copts.bzl", "copts")
load("@ecsact_rt_entt//runtime:index.bzl", "ecsact_entt_runtime")
load("@rules_cc//cc:defs.bzl", "cc_test")
load("@rules_ecsact//ecsact:defs.bzl", "ecsact_codegen")

def rt_entt_test(name = None):
    ecsact_codegen(
        name = "ecsact_cc_system_impl_srcs",
        srcs = ["{}.ecsact".format(name)],
        output_directory = "_ecsact_cc_system_impl_srcs",
        plugins = [
            "@ecsact_lang_cpp//cpp_systems_source_codegen",
        ],
    )

    ecsact_entt_runtime(
        name = "{}_test_runtime".format(name),
        srcs = ["{}.ecsact".format(name)],
        ECSACT_ENTT_RUNTIME_PACKAGE = "::{}::package".format(name),
        ECSACT_ENTT_RUNTIME_USER_HEADER = "{}.ecsact.meta.hh".format(name),
        system_impls = ["dynamic"],
    )

    cc_test(
        name = "{}_test".format(name),
        srcs = [
            ":{}_test.cc".format(name),
            ":ecsact_cc_system_impl_srcs",
        ],
        args = ["--gtest_catch_exceptions=0"],
        copts = copts,
        deps = [
            ":{}_test_runtime".format(name),
            "@googletest//:gtest",
            "@googletest//:gtest_main",
        ],
    )

    native.alias(
        name = name,
        actual = ":{}_test".format(name),
    )
