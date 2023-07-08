{
  "conditions": [
    ["OS=='win'", {
          "target_defaults": {
            "default_configuration": "Release_x64",
            "msbuild_toolset":"CTP_Nov2013",
            "msvs_settings": {
              "VCCLCompilerTool": {
                "ExceptionHandling": 1, # /EHsc
                "RuntimeTypeInfo": "true" # /GR
              }
            },
            "configurations": {
              "Debug_Win32": {
                "msvs_configuration_platform": "Win32",
                "defines": [ "DEBUG","_DEBUG"],
                "msvs_settings": {
                  "VCCLCompilerTool": {
                    "RuntimeLibrary": "1", # static debug /MTd
                    "Optimization": 0, # /Od, no optimization
                    "MinimalRebuild": "false",
                    "OmitFramePointers": "false",
                    "BasicRuntimeChecks": 3 # /RTC1
                  }
                }
              },
              "Debug_x64": {
                "msvs_configuration_platform": "x64",
                "defines": [ "DEBUG","_DEBUG"],
                "msvs_settings": {
                  "VCCLCompilerTool": {
                    "RuntimeLibrary": "1", # static debug /MTd
                    "Optimization": 0, # /Od, no optimization
                    "MinimalRebuild": "false",
                    "OmitFramePointers": "false",
                    "BasicRuntimeChecks": 3 # /RTC1
                  }
                }
              },
              "Release_Win32": {
                "msvs_configuration_platform": "Win32",
                "defines": [ "NDEBUG"],
                "msvs_settings": {
                  "VCCLCompilerTool": {
                    "RuntimeLibrary": 0, # static release
                    "Optimization": 3, # /Ox, full optimization
                    "FavorSizeOrSpeed": 1, # /Ot, favour speed over size
                    "InlineFunctionExpansion": 2, # /Ob2, inline anything eligible
                    "WholeProgramOptimization": "true", # /GL, whole program optimization, needed for LTCG
                    "OmitFramePointers": "true",
                    "EnableFunctionLevelLinking": "true",
                    "EnableIntrinsicFunctions": "true",
                    "AdditionalOptions": [
                      "/MP", # compile across multiple CPUs
                    ],
                    "DebugInformationFormat": "0"
                  },
                  "VCLibrarianTool": {
                    "AdditionalOptions": [
                      "/LTCG" # link time code generation
                    ],
                  },
                  "VCLinkerTool": {
                    "LinkTimeCodeGeneration": 1, # link-time code generation
                    "OptimizeReferences": 2, # /OPT:REF
                    "EnableCOMDATFolding": 2, # /OPT:ICF
                    "LinkIncremental": 1, # disable incremental linking
                    "GenerateDebugInformation": "false"
                  }
                }
              },
              "Release_x64": {
                "msvs_configuration_platform": "x64",
                "defines": [ "NDEBUG"],
                "msvs_settings": {
                  "VCCLCompilerTool": {
                    "RuntimeLibrary": 0, # static release
                    "Optimization": 3, # /Ox, full optimization
                    "FavorSizeOrSpeed": 1, # /Ot, favour speed over size
                    "InlineFunctionExpansion": 2, # /Ob2, inline anything eligible
                    "WholeProgramOptimization": "true", # /GL, whole program optimization, needed for LTCG
                    "OmitFramePointers": "true",
                    "EnableFunctionLevelLinking": "true",
                    "EnableIntrinsicFunctions": "true",
                    "AdditionalOptions": [
                      "/MP", # compile across multiple CPUs
                    ],
                    "DebugInformationFormat": "0"
                  },
                  "VCLibrarianTool": {
                    "AdditionalOptions": [
                      "/LTCG" # link time code generation
                    ],
                  },
                  "VCLinkerTool": {
                    "LinkTimeCodeGeneration": 1, # link-time code generation
                    "OptimizeReferences": 2, # /OPT:REF
                    "EnableCOMDATFolding": 2, # /OPT:ICF
                    "LinkIncremental": 1, # disable incremental linking
                    "GenerateDebugInformation": "false"
                  }
                }
              }
            }
          }
    }, {
        "target_defaults": {
            "default_configuration": "Release",
            "xcode_settings": {
              "CLANG_CXX_LIBRARY": "libc++",
              "CLANG_CXX_LANGUAGE_STANDARD":"c++11",
              "GCC_VERSION": "com.apple.compilers.llvm.clang.1_0",
            },
            "cflags_cc": ["-std=c++11"],
            "configurations": {
                "Debug": {
                    "defines": [
                        "DEBUG"
                    ],
                    "xcode_settings": {
                        "GCC_OPTIMIZATION_LEVEL": "0",
                        "GCC_GENERATE_DEBUGGING_SYMBOLS": "YES",
                        "OTHER_CPLUSPLUSFLAGS": [ "-Wall", "-Wextra", "-pedantic", "-g", "-O0" ]
                    }
                },
                "Release": {
                    "defines": [
                        "NDEBUG"
                    ],
                    "xcode_settings": {
                        "GCC_OPTIMIZATION_LEVEL": "3",
                        "GCC_GENERATE_DEBUGGING_SYMBOLS": "NO",
                        "DEAD_CODE_STRIPPING": "YES",
                        "GCC_INLINES_ARE_PRIVATE_EXTERN": "YES",
                        "OTHER_CPLUSPLUSFLAGS": [ "-Wall", "-Wextra", "-pedantic", "-O3" ]
                    }
                }
            }
        }
    }]
  ]
}

