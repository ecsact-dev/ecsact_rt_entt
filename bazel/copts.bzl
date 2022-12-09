"""
"""

copts = select({
    "@bazel_tools//tools/cpp:msvc": ["/std:c++20", "/Zc:preprocessor", "/permissive-", "/diagnostics:caret"],
    "//conditions:default": ["-std=c++20"],
})
