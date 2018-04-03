cc_library(
    name = "openlocationcode",
    srcs = [
        "openlocationcode.cc",
    ],
    hdrs = [
        "codearea.h",
        "openlocationcode.h",
    ],
    copts = ["-pthread"],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    deps = [
        ":codearea",
    ],
)

cc_library(
    name = "codearea",
    srcs = [
        "codearea.cc",
    ],
    hdrs = [
        "codearea.h",
    ],
    visibility = ["//visibility:private"],
)

cc_test(
    name = "openlocationcode_test",
    size = "small",
    srcs = ["openlocationcode_test.cc"],
    copts = [
        "-pthread",
        "-Iexternal/gtest/include",
    ],
    data = [
        "//test_data",
    ],
    linkopts = ["-pthread"],
    deps = [
        ":openlocationcode",
        "@gtest//:main",
    ],
)

cc_binary(
    name = "openlocationcode_example",
    srcs = [
        "openlocationcode_example.cc",
    ],
    deps = [
        ":openlocationcode",
    ],
)
