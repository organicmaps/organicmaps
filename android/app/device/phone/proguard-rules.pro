# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}

# Uncomment this to preserve the line number information for
# debugging stack traces.
-keepattributes SourceFile,LineNumberTable

# If you keep the line number information, uncomment this to
# hide the original source file name.
#-renamesourcefileattribute SourceFile

# For Guava used by Android Auto
-dontwarn java.lang.reflect.AnnotatedType

# Disable obfuscation since it is open-source app.
-dontobfuscate
# R8 crypts the source line numbers in all log messages.
# https://github.com/organicmaps/organicmaps/issues/6559#issuecomment-1812039926
-dontoptimize
