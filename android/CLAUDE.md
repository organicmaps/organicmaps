# Android-Specific Agent Instructions

## Project structure
- `app/` -- main application (Activities, Fragments, UI)
- `sdk/` -- core SDK wrapping C++ framework via JNI
- `libs/car/` -- Android Auto / Automotive screens
- `libs/` -- feature libraries (api, branding, downloader, routing, utils)
- `sdk/car/` -- SDK Car module; `sdk/widgets/` -- lane and speed limit widgets

## Build variants
- Flavors: `google` (Play Store), `fdroid`, `web`, `huawei`
- Types: `debug`, `release`, `beta`
- Architecture flags: `-Parm64` (default), `-Parm32`, `-Px86`, `-Px86_64`
- Build: `./gradlew assembleGoogleDebug -Parm64`
- Dependency versions: `gradle/libs.versions.toml`

## JNI bridge pattern
Java native methods in `Framework.java` / `OrganicMaps.java` map to C++ in `sdk/src/main/cpp/`:
```java
// Java side (sdk/src/main/java/app/organicmaps/sdk/Framework.java):
public static native int nativeGetDrawScale();
```
```cpp
// C++ side (sdk/src/main/cpp/...):
JNIEXPORT jint Java_app_organicmaps_sdk_Framework_nativeGetDrawScale(JNIEnv * env, jclass)
{
  return static_cast<jint>(GetFramework().GetDrawScale());
}
// String conversion: jni::ToNativeString(env, jstring), jni::ToJavaString(env, char const *)
// JNI helpers: sdk/src/main/cpp/app/organicmaps/sdk/core/jni_helper.hpp
```

## Activity lifecycle
- Extend `BaseMwmFragmentActivity` and override `onSafeCreate()` (not `onCreate()`)
  -- ensures C++ core is initialized before activity code runs
- Use `BaseMwmFragment` with `OnBackPressListener` interface
- Thread safety: use `Handler(Looper.getMainLooper())` for UI updates from native callbacks

## Key classes
- `MwmApplication` -- Application singleton, lifecycle management
- `SplashActivity` -- startup/initialization entry point
- `MwmActivity` -- main map activity (hosts fragments for search, routing, editor, etc.)
- `Framework.java` -- 200+ native methods bridging to C++ Framework
- `OrganicMaps.java` -- SDK initialization and platform setup
- `Map.java` -- surface rendering, touch events, widget management

## Android Auto / Automotive
- Screen-based navigation (not Activity-based): `MapScreen`, `SearchScreen`, `PlaceScreen`
- Uses AndroidX Car App library (`androidx.car.app`)
- `CarAppServiceBase` / `CarAppSessionBase` -- service and session lifecycle
- Separate manifest for car permissions (`NAVIGATION_TEMPLATES`, `ACCESS_SURFACE`)

## Testing
- JUnit 4 with Mockito
- Unit tests: `app/src/test/java/`, `sdk/src/test/java/`
- Instrumentation tests: `sdk/src/androidTest/java/`

## Important notes
- Minimum SDK: 21; target SDK: latest stable
- Java 17 source/target; Kotlin only enabled with Firebase
- NDK version: 29+; CMake: 3.22.1+
- Deep link schemes: `geo://`, `om://`, `ge0://`, `ge0.me` (HTTP/HTTPS)
- Permissions validated at build time via `permission-checker.gradle`
- ProGuard: obfuscation disabled (`-dontobfuscate`), line numbers preserved
- When editing translations in `app/src/main/res/values*/strings.xml` or store metadata, follow the translation rules in `data/CLAUDE.md`
