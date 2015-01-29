1. Include these files in your project by adding them into the build.gradle:
```
android {
  sourceSets.main {
    java.srcDirs = ['your_java_sources', 'Alohalytics/src/android/java']
    jni.srcDirs = ['Alohalytics/src', 'Alohalytics/src/android/jni']
  }
  defaultConfig {
    ndk {
      moduleName 'alohalytics'
      stl 'c++_static'
      cFlags '-frtti -fexceptions'
      ldLibs 'log', 'atomic'
    }
  }
}
```
But if you use custom Android.mk file for your sources, add these lines there (instead of jni.srcDirs and ndk block above):
```
  LOCAL_SRC_FILES += Alohalytics/src/android/jni/jni_alohalytics.cc \
                     Alohalytics/src/cpp/alohalytics.cc
```
2. Modify your AndroidManifest as in Alohalytics/src/android/AndroidManifest.xml
3. Insert ```System.loadLibrary("alohalytics");``` call if you don't load your own jni library. You can put it into your main activity:
```
  static {
    System.loadLibrary("alohalytics");
  }
```
