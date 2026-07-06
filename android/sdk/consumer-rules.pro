# R8/ProGuard rules applied automatically to apps consuming the SDK.
#
# The native code (JNI) looks up SDK classes, methods and fields by name,
# so renaming or stripping them breaks the SDK at runtime in minified builds.
-keep class app.organicmaps.sdk.** { *; }

# Keep the names of native methods everywhere (matches the default
# proguard-android-optimize.txt, but do not rely on consumers using it).
-keepclasseswithmembernames,includedescriptorclasses class * {
    native <methods>;
}
