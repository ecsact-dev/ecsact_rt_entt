load("@rules_ecsact//ecsact:defs.bzl", "ecsact_codegen")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@ecsact_rt_entt//bazel:copts.bzl", "copts")

def ecsact_entt_runtime(name, srcs = [], deps = [], system_impls = [], tags = [], ECSACT_ENTT_RUNTIME_USER_HEADER = None, ECSACT_ENTT_RUNTIME_PACKAGE = None, **kwargs):
    """
    """

    ecsact_codegen(
        name = "%s__public_hdrs" % name,
        srcs = srcs,
        tags = tags,
        plugins = [
            "@ecsact//codegen_plugins:cpp_header",
            "@ecsact//codegen_plugins:cpp_systems_header",
            "@ecsact//codegen_plugins:systems_header",
            "@ecsact//codegen_plugins:cpp_meta_header",
        ],
        **kwargs
    )

    cc_library(
        name = "%s__public_cc" % name,
        hdrs = [":%s__public_hdrs" % name],
        tags = tags,
        copts = copts,
        strip_include_prefix = "%s__public_hdrs" % name,
        deps = [
        ],
        **kwargs
    )

    _cc_local_defines = [
        "ECSACT_ENTT_RUNTIME_USER_HEADER=\\\"{}\\\"".format(ECSACT_ENTT_RUNTIME_USER_HEADER),
        "ECSACT_ENTT_RUNTIME_PACKAGE={}".format(ECSACT_ENTT_RUNTIME_PACKAGE),
    ]

    allowed_system_impls = [
        "dynamic",
        "static",
    ]

    for system_impl in system_impls:
        found_valid = False
        for allowed_system_impl in allowed_system_impls:
            if system_impl == allowed_system_impl:
                found_valid = True
                _cc_local_defines.append("ECSACT_ENTT_RUNTIME_%s_SYSTEM_IMPLS" % system_impl.upper())
                break
        if not found_valid:
            fail("Invalid value in system_impls attribute '%s'. Allowed values: %s" % (system_impl, ", ".join(allowed_system_impls)))

    if len(system_impls) == 0:
        fail("ecsact_entt_runtime: system_impls must contain at least one of the following: %s" % ", ".join(allowed_system_impls))

    _cc_srcs = [
        "@ecsact_rt_entt//runtime:sources",
    ]

    # keep sorted
    _cc_deps = [
        "@boost//libs/mp11",
        "@com_github_skypjack_entt//:entt",
        "@ecsact_rt_entt//:lib",
        "@ecsact_runtime//:core",
        "@ecsact_runtime//:dynamic",
        "@ecsact_runtime//:lib",
        "%s__public_cc" % name,
    ]

    cc_library(
        name = name,
        defines = _cc_local_defines + [
            "ECSACT_CORE_API=",
            "ECSACT_DYNAMIC_API=",
        ],
        copts = copts,
        tags = tags,
        deps = _cc_deps + deps,
        hdrs = [":%s__public_hdrs" % name],
        srcs = _cc_srcs,
        **kwargs
    )

    cc_binary(
        name = "%s_shared" % name,
        linkshared = True,
        tags = tags,
        local_defines = _cc_local_defines + [
            "ECSACT_CORE_API_EXPORT",
            "ECSACT_DYNAMIC_API_EXPORT",
        ],
        copts = copts,
        srcs = _cc_srcs,
        deps = _cc_deps + deps,
        **kwargs
    )
