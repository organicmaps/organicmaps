#!/usr/bin/env python3
"""Build isolated ARM64 simulator probes without changing tracked sources."""
import argparse
import plistlib
import shutil
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--repo", type=Path, required=True)
parser.add_argument("--products", type=Path, required=True,
                    help="Debug-iphonesimulator products from an OMaps build")
args = parser.parse_args()
repo, products = args.repo.resolve(), args.products.resolve()
scratch = Path(__file__).resolve().parent
base = "7d9cca20d11befd6fe97acbd5b7a065fd3916b54"
subprocess.run(["git", "-C", str(repo), "diff", "--quiet", base, "--", "libs", "xcode", "iphone", "3party", "data"],
               check=True)
sdk = subprocess.check_output(["xcrun", "--sdk", "iphonesimulator", "--show-sdk-path"], text=True).strip()
clang = subprocess.check_output(["xcrun", "--sdk", "iphonesimulator", "--find", "clang++"], text=True).strip()

includes = [repo, repo / "libs", repo / "libs/map", repo / "3party/boost_headers", repo / "3party/glaze/include",
            repo / "3party/utfcpp/source", repo / "3party/glm", repo / "3party/expat/lib",
            repo / "3party/pugixml/pugixml/src"]
flags = [clang, "-target", "arm64-apple-ios17.0-simulator", "-isysroot", sdk, "-std=c++23",
         "-g", "-O0", "-ffast-math", "-fvisibility=hidden", "-fvisibility-inlines-hidden", "-fblocks",
         "-DDEBUG", "-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE",
         "-DCOREVIDEO_SILENCE_GL_DEPRECATION", "-DGLES_SILENCE_DEPRECATION"]
for directory in includes:
    flags += ["-I", str(directory)]

overlay = scratch / "include"
for name in ["map/routing_manager.hpp", "routing/index_router.hpp"]:
    path = overlay / name
    path.parent.mkdir(parents=True, exist_ok=True)
    # Expose only these classes to the probe; dependency headers keep normal access.
    path.write_text((repo / "libs" / name).read_text().replace("private:", "public:"))

def compile_source(source, output, extra=()):
    subprocess.run([clang] + list(extra) + flags[1:] + ["-c", str(source), "-o", str(output)], check=True)

main_object = scratch / "main.o"
compile_source(scratch / "main.mm", main_object, ["-fobjc-arc", "-I", str(overlay)])

original_async = (repo / "libs/routing/async_router.cpp").read_text()
original_session = (repo / "libs/routing/routing_session.cpp").read_text()
original_manager = (repo / "libs/map/routing_manager.cpp").read_text()

def replace_once(source, old, new):
    assert source.count(old) == 1, f"Source changed; cannot safely apply probe: {old!r}"
    return source.replace(old, new, 1)

for fixed in [False, True]:
    name = "fixed" if fixed else "baseline"
    directory = scratch / name
    directory.mkdir(exist_ok=True)
    async_source = original_async
    session_source = original_session
    manager_source = original_manager
    if fixed:
        clear = ("      if (m_clearState && m_router)\n      {\n"
                 "        m_router->ClearState();\n        m_clearState = false;\n      }\n\n")
        async_source = replace_once(async_source, clear, "")
        async_source = replace_once(async_source, "      if (!m_hasRequest)\n        continue;\n", "")
        async_source = replace_once(async_source, "    bool hasRequest = m_hasRequest;\n",
                                    "\n".join(line[2:] for line in clear.split("\n")) +
                                    "    bool hasRequest = m_hasRequest;\n")
        session_source = replace_once(session_source, "  RemoveRoute();\n  SetState(SessionState::RouteNotStarted);",
                                      "  auto const passed = m_checkpoints.GetPassedIdx();\n"
                                      "  for (auto & variant : result->m_routes)\n"
                                      "    if (variant.GetCurrentSubrouteIdx() < passed)\n"
                                      "      variant.SetCurrentSubrouteIdx(passed);\n"
                                      "  RemoveRoute();\n  SetState(SessionState::RouteNotStarted);")
        session_source = replace_once(session_source, "    // Cancel current route building.\n    m_router->ClearState();\n", "")
        manager_source = replace_once(manager_source, "    if (!hasAlternatives)\n      return;",
                                      "    if (!hasAlternatives || m_routingSession.IsFollowing())\n      return;")
    async_source = replace_once(async_source, "namespace routing\n{",
                                "namespace simulator_repro { void BeforePickingRequest(); }\n\nnamespace routing\n{")
    async_source = replace_once(async_source, "    CalculateRoute();\n",
                                "    simulator_repro::BeforePickingRequest();\n    CalculateRoute();\n")
    objects = [main_object]
    for stem, source in [("async_router", async_source), ("routing_session", session_source),
                         ("routing_manager", manager_source)]:
        path = directory / (stem + ".cpp")
        path.write_text(source)
        output = directory / (stem + ".o")
        compile_source(path, output)
        objects.append(output)
    bundle = directory / "RoutingRepro.app"
    bundle.mkdir(exist_ok=True)
    executable = bundle / "RoutingRepro"
    libraries = sorted(products.glob("*.a"))
    assert libraries and (products / "libmap.a") in libraries
    frameworks = ["UIKit", "Foundation", "CoreFoundation", "CoreLocation", "CoreGraphics", "QuartzCore",
                  "Metal", "MetalKit", "OpenGLES", "CoreVideo", "SystemConfiguration", "Network", "Security",
                  "CFNetwork", "AVFoundation"]
    link = flags + [str(p) for p in objects + libraries] + ["-F", str(products), "-framework", "minizip",
            "-lz", "-liconv", "-lbz2", "-Wl,-rpath,@executable_path/Frameworks", "-o", str(executable)]
    for framework in frameworks:
        link += ["-framework", framework]
    subprocess.run(link, check=True)
    with (bundle / "Info.plist").open("wb") as stream:
        plistlib.dump({"CFBundleIdentifier": "app.organicmaps.routing-repro." + name,
                      "CFBundleExecutable": "RoutingRepro", "CFBundleName": "Routing Reproduction",
                      "CFBundlePackageType": "APPL", "CFBundleVersion": "1",
                      "CFBundleShortVersionString": "1.0", "MinimumOSVersion": "17.0",
                      "UIDeviceFamily": [1, 2], "UILaunchScreen": {},
                      "UIApplicationSceneManifest": {"UIApplicationSupportsMultipleScenes": False,
                          "UISceneConfigurations": {"UIWindowSceneSessionRoleApplication": [{
                              "UISceneConfigurationName": "Reproduction",
                              "UISceneDelegateClassName": "ReproSceneDelegate"}]}}}, stream)
    embedded = bundle / "Frameworks/minizip.framework"
    embedded.parent.mkdir(exist_ok=True)
    shutil.copytree(products / "minizip.framework", embedded, dirs_exist_ok=True)
    for resource in ["classificator.txt", "types.txt", "drules_default.bin", "drules_outdoors.bin",
                     "drules_vehicle.bin", "drules_merged.bin"]:
        shutil.copy2(repo / "data" / resource, bundle / resource)
    subprocess.run(["codesign", "--force", "--sign", "-", str(embedded)], check=True)
    subprocess.run(["codesign", "--force", "--sign", "-", str(bundle)], check=True)
    print("Built ARM64 simulator probe:", bundle, flush=True)
