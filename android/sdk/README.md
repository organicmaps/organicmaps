# Organic Maps Android SDK

The SDK embeds the Organic Maps renderer, offline search, downloads, bookmarks,
and routing in an Android application. Android 5.0 (API 21) is the minimum runtime.
Use the compile SDK from `propCompileSdkVersion` in [gradle.properties](../gradle.properties)
and the build JDK configured in the [SDK workflow](../../.github/workflows/android-sdk.yaml).
Java source and target compatibility remain 17.

## Dependencies and packaging

Use the same version for every `app.organicmaps.sdk` dependency:

```kotlin
dependencies {
    implementation("app.organicmaps.sdk:sdk:<version>")
    implementation("app.organicmaps.sdk:maps-world:<version>")
    coreLibraryDesugaring("com.android.tools:desugar_jdk_libs:<desugar-version>")
}

android {
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
        isCoreLibraryDesugaringEnabled = true
    }
    androidResources {
        noCompress += listOf("png", "mwm", "ttf", "packed_polygons.bin")
    }
}
```

Use the desugaring version from [libs.versions.toml](../gradle/libs.versions.toml).
The SDK needs in-place access to map files, fonts, and packed polygons, so leave them
uncompressed. PNG images are already compressed; storing them avoids redundant deflation.
The main AAR contains native libraries and the
engine's styles, fonts, and classifiers. `maps-world` adds overview maps; detailed
offline search and routing require downloaded region maps.

| Artifact | Purpose |
|---|---|
| `sdk` | Core framework, rendering, downloads, search, routing, bookmarks |
| `maps-world` | Bundled World and WorldCoasts overview maps |
| `location-core` | Location-provider interfaces, exposed transitively by `sdk` |
| `location-google` | Optional Google Play Services location provider |
| `location-foss` | Alternative microG location-provider implementation |
| `car` | Android Auto and Automotive rendering integration |
| `widgets-lanes` | Navigation lane widget |
| `widgets-speedlimit` | Speed-limit widget |

Choose at most one of `location-google` and `location-foss`: they provide the same
implementation classes. Without either, the SDK uses Android's native location
providers. Wear bridge modules are not part of this publication set.

## Initialization and activity recreation

Loading the `OrganicMaps` class loads the native library. Create one instance in
your `Application` to configure application-private settings. Call `init()` on
the main thread to create the framework and load maps asynchronously.

The first successful `init(callback)` call returns `true`. The callback runs on
the main thread after maps, search, editor state, and routing initialization are
ready. Later calls return `false` without invoking their callbacks, including
while the first load is pending. `arePlatformAndCoreInitialized()` also does not
mean map loading has completed.

Keep readiness in the application, for example in a `LiveData<Boolean>`, and let
activities observe it with their own lifecycle. Register the original completion
callback against that process-owned state rather than an activity. Handle
`IOException` from `init()` by displaying an initialization error and avoiding
native API calls. The constructor throws `AssertionError` if its private settings
directory cannot be created. Keeping readiness in the application also handles
rotation during the first map load.

After readiness is reported, inflate a `MapView`, construct a `MapController`
before attaching the view to a window, and register the controller:

```java
MapController controller = new MapController(mapView, sdk.getLocationHelper(),
    renderingListener, unsupportedGraphicsCallback, false);
getLifecycle().addObserver(controller);
```

The controller forwards surface-related lifecycle events and releases rendering
callbacks when destroyed. The native framework is process-wide and outlives an
activity. `MapRenderingListener.onRenderingCreated()` reports engine creation;
`onRenderingRestored()` reports a surface resize or restoration. These are
separate from the completion of initial map loading.

Use `MapStyle.mark()` before renderer creation and `MapStyle.set()` when changing
an existing renderer. Forward system-bar and cutout insets through the controller's
widget-offset methods: compass offsets are from the right and top edges; bottom
widget offsets are from the left and bottom edges. Values are screen pixels.

## Location and selection

Location permissions in the SDK manifest are merged into the host, but the host
must request runtime permissions. Register a `LocationState.ModeChangeListener`
after framework initialization, update the position button, and start or stop
`sdk.getLocationHelper()` according to the mode and granted permissions.
`resumeLocationInForeground()` checks permission, renderer, and mode before
starting updates. Stop updates when the host no longer needs them.
Listener registration may precede renderer creation and does not replay the
current mode. Read or change the mode only after the renderer exists.

Position mode and place-selection listeners are process-wide single listeners.
Register them for the active host and remove them when that host ends. Selection
callbacks provide `PlacePageData`; inspect its type before treating it as a
`MapObject`. A navigation application that continues in the background must own
its foreground service, notification, and background-location policy.

## Downloads and offline search

Use `SearchEngine.searchMaps()` to find catalogue regions. The returned region ID
can be passed to `MapManager.startDownload()`. Subscribe before requesting the
download and retain the slot returned by `nativeSubscribe()`; unsubscribe that
slot when its UI stops. Refresh the current `CountryItem` when UI resumes because
downloads can continue independently. Progress byte counts may have a zero total.
Check available storage, expose errors/retry/cancellation, and obtain the user's
agreement before starting a network download.

For POI search, add a `SearchListener`, then call `SearchEngine.INSTANCE.search()`.
Supply a new monotonically increasing request timestamp and ignore callbacks for
superseded requests. Each result update replaces the previous snapshot. If UI
filters suggestions, preserve each result's original native index for
`showResult()` or `selectResult()`. Cancelling a request does not retract callbacks
already queued. Remove listeners and invalidate request IDs when the host stops.

Coordinates used for search hints and `MapObject` positions are WGS84 degrees.
APIs accepting Mercator coordinates or screen pixels identify those explicitly.

## Routing

Attach one `RoutingController.Container` and call `restore()` to replay the current
state. Use `prepare(origin, destination, profile)` to build a plan asynchronously.
Implement `showRoutePlan(show, completion)` and invoke `completion` once the plan
UI is ready, even if your controls are always visible: the controller starts its
build from that continuation.
Implement progress, build success, and `onCommonBuildError()` callbacks. A failed
build can return region IDs that must be downloaded before rebuilding. World
overview maps alone do not contain the road graph for detailed routing.

Enable the start action only after a successful build in planning mode. Starting
navigation requires a usable current position and location updates. Stop or
cancel through the controller. Detaching the UI alone does not cancel navigation.
Call `onSaveState()` when the host saves its UI and `restore()` when it is active
again. Route state survives activity recreation; the host still owns its controls,
listener cleanup, and any background service.

## Release consumers

The SDK packages consumer R8 rules for JNI access by name. Test a minified release
APK when changing the Java/JNI boundary. Resource shrinking needs separate checks:
resources such as bookmark icons are resolved with `Resources.getIdentifier()`
(see `Icon` and `Utils`). Keep the map attribution visible and
include the project's existing [code](../../LICENSE) and
[binary-data](../../DATA_LICENSE.txt) licensing information in your integration.
