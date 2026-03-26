# iOS Bridge Navigation

This table is a compact search map for LLM agents investigating how the iOS codebase reaches the C++ core.

Two architectural rules matter when navigating this area:
- `CoreApi` is the reusable iOS-facing bridge around the C++ `Framework`.
- `Maps` is not only UI; it also contains app-specific ObjC++ seams that talk to C++ directly.

| Component | Path | Focus |
|---|---|---|
| iOS Workspace | `xcode/omim.xcworkspace` | Top-level wiring for iOS projects and C++ library projects; starting point for target relationships |
| CoreApi Target | `iphone/CoreApi/CoreApi.xcodeproj` | Separate `CoreApi.framework` target; reusable iOS bridge layer over the C++ core |
| CoreApi Public Surface | `iphone/CoreApi/CoreApi/CoreApi.h`, `iphone/CoreApi/CoreApi.modulemap` | Umbrella/module entry points; first files to open for exported bridge APIs |
| CoreApi Framework Bridge | `iphone/CoreApi/CoreApi/Framework/` | Owns `GetFramework()` and `MWMFrameworkHelper`; highest-value bridge entry point into `map/framework.hpp` |
| CoreApi Domain Wrappers | `iphone/CoreApi/CoreApi/{Bookmarks,PlacePageData,Storage,Search,Traffic,DeepLink,Formatting,Common,NetworkPolicy,Logger}/` | ObjC/Swift-friendly wrappers around core subsystems; `.mm` files convert Foundation types to C++ types |
| CoreApi Conversion Helpers | `iphone/CoreApi/CoreApi/**/*+Core.h` | Internal adapter layer; categories/init helpers that expose C++-backed constructors and converters |
| Maps App Target | `iphone/Maps/Maps.xcodeproj` | Main app target (`OMaps`); Swift + ObjC++ app shell that consumes `CoreApi` and core static libs |
| Maps Swift Bridge Surface | `iphone/Maps/Bridging-Header.h` | Swift import boundary for the app; imports `<CoreApi/CoreApi.h>` plus app-specific ObjC/ObjC++ headers |
| Maps App Bootstrap | `iphone/Maps/{main.mm,Classes/MapsAppDelegate.mm,Classes/MapViewController.mm}` | App startup, map screen shell, and major runtime bridge entry points |
| Maps Core Wrappers | `iphone/Maps/Core/` | App-specific wrappers/listeners for routing, search, location, settings, framework observers, and deep links |
| Maps UI Feature Layers | `iphone/Maps/{UI,Bookmarks,Classes/CustomViews}/` | Feature UI modules; mostly consume `CoreApi` or Maps wrapper objects rather than raw C++ |
| Maps Direct C++ Touchpoints | `iphone/Maps/**/*.mm` and selected headers with core includes | App-only ObjC++ seams that bypass `CoreApi` for rendering, editor, routing, storage, and framework listeners |
| Widget Extension | `iphone/Maps/OMapsWidgetExtension/` | Separate extension/UI target; useful when tracing non-main-app iOS entry points |
| iOS Tests | `iphone/Maps/Tests/` | Unit tests for iOS wrappers, feature modules, and bridge-adjacent behavior |

## Validation Anchors

- `xcode/omim.xcworkspace` includes both `iphone/CoreApi/CoreApi.xcodeproj` and `iphone/Maps/Maps.xcodeproj`.
- `iphone/Maps/Maps.xcodeproj/project.pbxproj` links `CoreApi.framework` and many C++ static libraries.
- `iphone/Maps/Bridging-Header.h` imports `<CoreApi/CoreApi.h>`.
- `iphone/CoreApi/CoreApi/Framework/Framework.h` wraps `map/framework.hpp`.
- Representative `.mm` files in both targets include either `<CoreApi/Framework.h>` or direct core headers.
