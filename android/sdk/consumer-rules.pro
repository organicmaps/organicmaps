# R8/ProGuard rules applied automatically to apps consuming the SDK.
#
# The native code (JNI) looks up SDK classes, methods and fields by name,
# so renaming or stripping them breaks the SDK at runtime in minified builds.
#
# TODO: Replace this blanket keep with targeted JNI rules; verify a minified
# consumer and compare APK size (https://github.com/organicmaps/organicmaps/issues/5324).
-keep class app.organicmaps.sdk.** { *; }

# JNI resolves runAsync with the okhttp3.Call return type and calls cancel() by name.
-keep interface okhttp3.Call {
    void cancel();
}
