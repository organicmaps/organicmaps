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

# For some unknown reason we couldn't find out, requests are not working properly
# when the app is shrinked and/or minified, so we keep all of these things out from R8 effects.
-keep,allowobfuscation,allowshrinking class kotlin.coroutines.Continuation
-keep,allowobfuscation,allowshrinking interface retrofit2.Call
-keep,allowobfuscation,allowshrinking class retrofit2.Response

-if interface * { @retrofit2.http.* public *** *(...); }
-keep,allowoptimization,allowshrinking,allowobfuscation class <3>

-keep class app.tourism.data.remote.** { *; }

-keep public class app.tourism.data.dto.** {
  public void set*(***);
  public *** get*();
  public protected private *;
}
-keep public class app.tourism.domain.models.** {
  public void set*(***);
  public *** get*();
  public protected private *;
}
