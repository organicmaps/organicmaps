# iOS-Specific Agent Instructions

## Project structure
- `Maps/` -- main iOS app (UI, Core services, resources, tests)
- `CoreApi/` -- Objective-C++ framework bridging C++ core to Swift (separate xcodeproj)
- `Chart/` -- chart component library (separate xcodeproj)
- Workspace: `xcode/omim.xcworkspace` (coordinates all projects)

## Subsystem entry points
- Location: `Maps/Core/Location/`; `MWMLocationManager`
- Search: `Maps/Core/Search/`, `Maps/UI/Search/`; `MWMSearch`, `SearchOnMapManager`
- Routing/navigation: `Maps/Core/Routing/`, `Maps/UI/NavigationDashboard/`; `MWMRouter`
- Track recording: `Maps/Core/TrackRecorder/`; `TrackRecordingManager`
- CarPlay: `Maps/Core/CarPlay/`, `Maps/UI/CarPlay/`; `CarPlayService`
- Storage/downloads: `Maps/Core/Storage/`, `Maps/UI/Downloader/`
- Bookmarks: `CoreApi/CoreApi/Bookmarks/`, `Maps/UI/Bookmarks/`; `MWMBookmarksManager`
- Place Page: `Maps/UI/PlacePage/`; `PlacePageBuilder`, `MWMPlacePageManager`
- Deep links: `Maps/Core/DeepLink/`
- iCloud sync: `Maps/Core/iCloud/`; `iCloudSynchronizaionManager`
- Text-to-speech: `Maps/Core/TextToSpeech/`
- Map rendering/view: `Maps/Core/MapRendering/`, `Maps/UI/Map/`; `EAGLView`, `MapViewController`
- Traffic/transit/isolines overlays: `CoreApi/CoreApi/Traffic/`; `MWMMapOverlayManager`
- Basic formatters: `CoreApi/CoreApi/Formatting/` for user-visible values:
  `DistanceFormatter`, `AltitudeFormatter`, `DurationFormatter`, `DateTimeFormatter`
  
## Bridging architecture
```
Swift services/UI or legacy ObjC -> ObjC/ObjC++ wrappers (`MWM*`) -> CoreApi -> C++ core
```
- `Maps/Bridging-Header.h` -- imports ObjC headers into Swift
- `CoreApi/CoreApi/Framework/MWMFrameworkHelper.h` -- public API surface for C++ Framework
- `CoreApi/CoreApi/Framework/Framework.h` -- raw C++ access via `GetFramework()`
- Use `@objc/@objcMembers` when Swift code must be visible to Objective-C
- Put C++-touching bridge code in `.mm` files, not Swift or plain `.m`
- If an ObjC bridge type exposes C++ types in public initializers/methods, declare them in a separate
  `Type+Core.h` `(Core)` category
- Keep C++ types out of Swift-visible headers; expose Swift-friendly ObjC wrappers instead

## Naming conventions
- Objective-C classes: `MWM` prefix (e.g., `MWMViewController`, `MWMRouter`, `MWMSettings`)
- Swift: PascalCase without prefix
- Localization: Swift `L("key")`, ObjC `L(@"key")`; keys are snake_case
- Constants: ObjC `kConstantName`; Swift `private enum Constants { static let ... }`
- Categories/Extensions: `ClassName+Extension.swift` or `ClassName+Category.m`

## UI patterns
- UIKit is the default. SwiftUI is currently limited to `OMapsWidgetExtension` Live Activity code.
- Prefer programmatic UIKit for new screens; keep existing storyboards/XIBs when editing legacy flows.
- Validate UI approaches for iPhone, iPad, and “Designed for iPad” on macOS.
- Common screen patterns:
  - Builder + View + Interactor + Presenter, sometimes Router (`BookmarksList`, `NavigationDashboard`, `SearchOnMap`)
  - MVC for simple or legacy screens
  - `ModalPresentationStepsController` + `ModalPresentationStep` for draggable modal sheets
    (`SearchOnMapViewController`)
- Base classes: `MWMViewController`, `MWMTableViewController`, `MWMTableViewCell`
- Theme system:
  - Add reusable styles to `Maps/Core/Theme/*StyleSheet.swift` (`IStyleSheet`)
  - Apply styles with `setStyle(...)`; use `setStyleAndApply(...)` only when the visible view must update immediately
  - `ThemeManager` controls app theme and Dynamic Type invalidation; `StyleManager` reapplies styles
- Fonts and Dynamic Type:
  - Prefer stylesheet font styles with `.dynamic(...)`; use `.fixed(...)` only when scaling is undesirable
  - For labels without a reusable style, use `setFontStyle(.regular14, color: .blackPrimary)`
  - Raw fonts from `UIFont+Fonts.swift` are static unless wrapped with `.dynamic`
- Colors/images live in `Colors.xcassets` and `Images.xcassets`; prefer typed accessors like
  `UIColor.blackPrimaryText` and `UIImage.icVoiceOn`

## Logging
- ObjC/ObjC++ logging: `LOG(LERROR, ("Error log"));`
- Swift logging: `LOG(.debug, "Debug log")`
- Levels: ObjC/ObjC++ `LDEBUG`, `LINFO`, `LWARNING`, `LERROR`, `LCRITICAL`; Swift `.debug`,
  `.info`, `.warning`, `.error`, `.critical`

## Xcode project files
- New iOS source/resource files usually must be added to `Maps.xcodeproj` or `CoreApi.xcodeproj`
- Do not hand-edit generated files or generated asset/color/image accessors
- Formatting is handled by the pre-commit hook; do not run formatters unless explicitly requested

## C++ callback bridging
- `MWMFrameworkListener.mm` routes C++ events to ObjC observers via `NSHashTable` (weak refs)
- Dispatch C++ callbacks to the main queue before notifying UI observers
- Never call C++ Framework methods on background threads without checking thread safety

## Build and validation
- Scheme: `OMaps`; workspace: `xcode/omim.xcworkspace`
- List installed iPhone simulators with `xcrun simctl list devices available | rg "iPhone"`
- Build with an installed iPhone simulator, preferably the latest available iPhone runtime/device:
  ```bash
  xcodebuild -workspace xcode/omim.xcworkspace \
    -scheme OMaps \
    -configuration Debug \
    -destination 'platform=iOS Simulator,name=<installed iPhone>' \
    build
  ```
- Build settings: iOS 15.0, macOS 10.15, C++23, Swift 5.5
- App/build constants live in `xcode/common.xcconfig`, `xcode/common-debug.xcconfig`, and
  `xcode/common-release.xcconfig`
- Entitlements: CarPlay, iCloud, Associated Domains (`applinks:omaps.app`), Push Notifications

## Testing
- XCTest framework
- Import: `@testable import Organic_Maps__Debug_`
- Tests in `Maps/Tests/` (Core, UI, Bookmarks, CarPlay)
- Mock objects follow `Mock*` naming convention

## Localization
- Language directories in `Maps/LocalizedStrings/`
- Widget extension: `Maps/OMapsWidgetExtension/` with LiveActivity support
- Do not hardcode user-facing strings
- For a new string key:
  1. Use a valid snake_case key
  2. Add it to `data/strings/strings.txt` with the `apple-maps` tag, value, and description
  3. Regenerate with `./tools/unix/generate_localizations.sh`
  4. Add translations only when explicitly requested by the user
- When editing store metadata in `metadata/` or localized strings, follow the translation rules in `data/CLAUDE.md`
