# iOS-Specific Agent Instructions

## Project structure
- `Maps/` -- main iOS app (Classes, UI, Core, Categories, Bookmarks, Tests)
- `CoreApi/` -- Objective-C++ framework bridging C++ core to Swift (separate xcodeproj)
- `Chart/` -- chart component library (separate xcodeproj)
- `DatePicker/` -- date picker component (separate xcodeproj)
- Workspace: `xcode/omim.xcworkspace` (coordinates all projects)

## Bridging architecture
```
Swift UI (ViewControllers) -> ObjC wrappers (MWM* classes) -> CoreApi framework -> C++ core
```
- `Maps/Bridging-Header.h` -- imports ObjC headers into Swift
- `CoreApi/CoreApi/Framework/MWMFrameworkHelper.h` -- public API surface for C++ Framework
- `CoreApi/CoreApi/Framework/Framework.h` -- raw C++ access via `GetFramework()`
- Use `@objc` when Swift code must be visible to Objective-C

## Naming conventions
- Objective-C classes: `MWM` prefix (e.g., `MWMViewController`, `MWMRouter`, `MWMSettings`)
- Swift: PascalCase without prefix
- Localization: `L("key")` function in `Maps/Common/Common.swift`
- Constants: `kConstantName` (ObjC) or `static let` (Swift)
- Categories/Extensions: `ClassName+Extension.swift` or `ClassName+Category.m`

## UI patterns
- Predominantly UIKit (not SwiftUI); mix of storyboards and programmatic UI
- VIPER architecture in Bookmarks module (Builder -> View/Presenter/Router/Interactor)
- Base classes: `MWMViewController`, `MWMTableViewController`, `MWMCollectionViewController`
- Theme system in `Maps/Core/Theme/` with `StyleSheet` protocol and renderers

## C++ callback bridging
- `MWMFrameworkListener.mm` routes C++ events to ObjC observers via `NSHashTable` (weak refs)
- Always dispatch to main queue: `dispatch_async(dispatch_get_main_queue(), ^{ ... })`
- Never call C++ Framework methods on background threads without checking thread safety

## Build configuration
- Deployment target: iOS 12.0, macOS 10.15
- C++ standard: C++23; Swift 5.5
- Scheme: `OMaps`; workspace: `xcode/omim.xcworkspace`
- Entitlements: CarPlay, iCloud, Associated Domains (`applinks:omaps.app`), Push Notifications

## Testing
- XCTest framework
- Import: `@testable import Organic_Maps__Debug_`
- Tests in `Maps/Tests/` (Core, UI, Bookmarks, CarPlay)
- Mock objects follow `Mock*` naming convention

## Localization
- Language directories in `Maps/LocalizedStrings/`
- Widget extension: `Maps/OMapsWidgetExtension/` with LiveActivity support
- When editing store metadata in `metadata/` or localized strings, follow the translation rules in `data/CLAUDE.md`
