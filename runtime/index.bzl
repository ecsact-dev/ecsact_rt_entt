load("@ecsact//:index.bzl", "ecsact_codegen")
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_binary")

def ecsact_entt_runtime(name, srcs = [], main = None, deps = [], ECSACT_ENTT_RUNTIME_USER_HEADER = None, ECSACT_ENTT_RUNTIME_PACKAGE = None, **kwargs):
    ecsact_codegen(
        name = "%s__private_srcs" % name,
        main = main,
        srcs = srcs,
        plugins = {
            "@ecsact//generator/cpp_systems/source/static": {},
        },
        **kwargs
    )

    ecsact_codegen(
        name = "%s__public_hdrs" % name,
        main = main,
        srcs = srcs,
        plugins = {
            "@ecsact//generator/cpp/header": {
                "constexpr_component_ids": True,
            },
            "@ecsact//generator/cpp_systems/header": {
                "constexpr_system_ids": True,
            },
            "@ecsact//generator/meta_cc": {},
        },
        **kwargs
    )

    cc_library(
        name = "%s__public_cc" % name,
        hdrs = ["%s__public_hdrs" % name],
        deps = [
            "@ecsact//lib:cc",
            "@ecsact//lib/runtime-cpp",
        ],
        **kwargs,
    )

    _cc_local_defines = [
        "ECSACT_ENTT_RUNTIME_USER_HEADER=\\\"{}\\\"".format(ECSACT_ENTT_RUNTIME_USER_HEADER),
        "ECSACT_ENTT_RUNTIME_PACKAGE={}".format(ECSACT_ENTT_RUNTIME_PACKAGE),
    ]

    _cc_srcs = [
        "@ecsact_entt//runtime:sources",
        ":%s__public_hdrs" % name,
        ":%s__private_srcs" % name,
    ]

    _cc_deps = [
        "@boost//libs/mp11",
        "@ecsact//lib/runtime",
        "@ecsact//lib/runtime-cpp",
        "@ecsact//runtime-util",
        "@com_github_skypjack_entt//:entt",
    ]

    cc_library(
        name = name,
        local_defines = _cc_local_defines + [
            "ECSACT_CORE_API=",
            "ECSACT_DYNAMIC_API=",
        ],
        deps = _cc_deps + deps,
        hdrs = [":%s__public_hdrs" % name],
        srcs = _cc_srcs,
        **kwargs
    )

    cc_binary(
        name = "%s_shared" % name,
        linkshared = True,
        tags = ["manual"],
        local_defines = _cc_local_defines + [
            "ECSACT_CORE_API_EXPORT",
            "ECSACT_DYNAMIC_API_EXPORT",
        ],
        srcs = _cc_srcs,
        deps = _cc_deps + deps,
        **kwargs
    )
