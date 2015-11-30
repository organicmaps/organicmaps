#include "jni_helper.hpp"
#include "logging.hpp"

#include "base/assert.hpp"
#include "std/vector.hpp"

static JavaVM * g_jvm = 0;
extern JavaVM * GetJVM()
{
  return g_jvm;
}

// TODO refactor cached jclass to smth more
// TODO finish this logic after refactoring
// cached classloader that can be used to find classes & methods from native threads.
//static shared_ptr<jobject> g_classLoader;
//static jmethodID g_findClassMethod;

// caching is necessary to create class from native threads
jclass g_indexClazz;

// @TODO remove after refactoring. Needed for NVidia code
void InitNVEvent(JavaVM * jvm);

extern "C"
{
  JNIEXPORT jint JNICALL
  JNI_OnLoad(JavaVM * jvm, void *)
  {
    g_jvm = jvm;
    jni::InitSystemLog();
    jni::InitAssertLog();

    JNIEnv * env = jni::GetEnv();
    // TODO
    // init classloader & findclass methodID.
//    auto randomClass = env->FindClass("com/mapswithme/maps/MapStorage");
//    jclass classClass = env->GetObjectClass(randomClass);
//    auto classLoaderClass = env->FindClass("java/lang/ClassLoader");
//    auto getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader",
//                                             "()Ljava/lang/ClassLoader;");
//    g_classLoader = jni::make_global_ref(env->CallObjectMethod(randomClass, getClassLoaderMethod));
//    ASSERT(*g_classLoader, ("Classloader can't be 0"));
//    g_findClassMethod = env->GetMethodID(classLoaderClass, "findClass",
//                                    "(Ljava/lang/String;)Ljava/lang/Class;");
//    ASSERT(g_findClassMethod, ("FindClass methodId can't be 0"));
    g_indexClazz = static_cast<jclass>(env->NewGlobalRef(env->FindClass("com/mapswithme/maps/MapStorage$Index")));
    ASSERT(g_indexClazz, ("Index class not found!"));

    return JNI_VERSION_1_6;
  }

  JNIEXPORT void JNICALL
  JNI_OnUnload(JavaVM *, void *)
  {
    g_jvm = 0;
    jni::GetEnv()->DeleteGlobalRef(g_indexClazz);
  }
} // extern "C"

namespace jni
{
  //
//  jclass FindClass(char const * name)
//  {
//    JNIEnv * env = GetEnv();
//    jstring className = env->NewStringUTF(name);
//    jclass clazz = static_cast<jclass>(GetEnv()->CallObjectMethod(*g_classLoader, g_findClassMethod, className));
//    env->DeleteLocalRef(className);
//    return clazz;
//  }

  jmethodID GetJavaMethodID(JNIEnv * env, jobject obj, char const * fn, char const * sig)
  {
    ASSERT(env, ("JNIEnv can't be 0"));
    ASSERT(obj, ("jobject can't be 0"));

    jclass cls = env->GetObjectClass(obj);
    ASSERT(cls, ("Can't get class: ", DescribeException()));

    jmethodID mid = env->GetMethodID(cls, fn, sig);
    ASSERT(mid, ("Can't get methodID", fn, sig, DescribeException()));

    env->DeleteLocalRef(cls);

    return mid;
  }

  jclass GetGlobalClassRef(JNIEnv * env, char const * sig)
  {
    jclass klass = env->FindClass(sig);
    ASSERT(klass, ("Can't get class : ", DescribeException()));
    return static_cast<jclass>(env->NewGlobalRef(klass));
  }

  string ToNativeString(JNIEnv * env, jstring str)
  {
    string result;
    char const * utfBuffer = env->GetStringUTFChars(str, 0);
    if (utfBuffer)
    {
      result = utfBuffer;
      env->ReleaseStringUTFChars(str, utfBuffer);
    }
    return result;
  }

  string ToNativeString(JNIEnv * env, jbyteArray const & bytes)
  {
    int const len = env->GetArrayLength(bytes);
    vector<char> buffer(len);
    env->GetByteArrayRegion(bytes, 0, len, reinterpret_cast<jbyte *>(buffer.data()));
    return string(buffer.data(), len);
  }

  jstring ToJavaString(JNIEnv * env, char const * s)
  {
    return env->NewStringUTF(s);
  }

  jclass GetStringClass(JNIEnv * env)
  {
    return env->FindClass(GetStringClassName());
  }

  char const * GetStringClassName()
  {
    return "java/lang/String";
  }

  JNIEnv * GetEnv()
  {
    JNIEnv * env;
    if (JNI_OK != g_jvm->GetEnv((void **)&env, JNI_VERSION_1_6))
    {
      ASSERT(false, ("Can't get JNIEnv. Was thread attached to JVM?"));
      return 0;
    }
    return env;
  }

  JavaVM * GetJVM()
  {
    ASSERT(g_jvm, ("JVM is not initialized"));
    return g_jvm;
  }

  struct global_ref_deleter
  {
    void operator()(jobject * ref)
    {
      jni::GetEnv()->DeleteGlobalRef(*ref);
      delete ref;
    }
  };

  shared_ptr<jobject> make_global_ref(jobject obj)
  {
    jobject * ref = new jobject;
    *ref = jni::GetEnv()->NewGlobalRef(obj);
    return shared_ptr<jobject>(ref, global_ref_deleter());
  }

  string DescribeException()
  {
    JNIEnv * env = jni::GetEnv();

    if (env->ExceptionCheck())
    {
      jthrowable e = env->ExceptionOccurred();

      // have to clear the exception before JNI will work again.
      env->ExceptionClear();

      jclass eclass = env->GetObjectClass(e);

      jmethodID mid = env->GetMethodID(eclass, "toString", "()Ljava/lang/String;");

      jstring jErrorMsg = (jstring) env->CallObjectMethod(e, mid);

      return ToNativeString(env, jErrorMsg);
    }

    return "";
  }

  jobject GetNewParcelablePointD(JNIEnv * env, m2::PointD const & point)
  {
    jclass klass = env->FindClass("com/mapswithme/maps/bookmarks/data/ParcelablePointD");
    ASSERT ( klass, () );
    jmethodID methodID = env->GetMethodID(
        klass, "<init>",
        "(DD)V");
    ASSERT ( methodID, () );

    return env->NewObject(klass, methodID,
                              static_cast<jdouble>(point.x),
                              static_cast<jdouble>(point.y));
  }

  jobject GetNewPoint(JNIEnv * env, m2::PointD const & point)
  {
    return GetNewPoint(env, m2::PointI(static_cast<int>(point.x), static_cast<int>(point.y)));
  }

  jobject GetNewPoint(JNIEnv * env, m2::PointI const & point)
  {
    jclass klass = env->FindClass("android/graphics/Point");
    ASSERT ( klass, () );
    jmethodID methodID = env->GetMethodID(
        klass, "<init>",
        "(II)V");
    ASSERT ( methodID, () );

    return env->NewObject(klass, methodID,
                              static_cast<jint>(point.x),
                              static_cast<jint>(point.y));
  }

  // TODO
  // make ScopedLocalRef wrapper similar to https://android.googlesource.com/platform/libnativehelper/+/jb-mr1.1-dev-plus-aosp/include/nativehelper/ScopedLocalRef.h
  // for localrefs automatically removed after going out of scope

  // This util method dumps content of local and global reference jni tables to logcat for debug and testing purposes
  void DumpDalvikReferenceTables()
  {
    JNIEnv * env = jni::GetEnv();
    jclass vm_class = env->FindClass("dalvik/system/VMDebug");
    jmethodID dump_mid = env->GetStaticMethodID(vm_class, "dumpReferenceTables", "()V");
    env->CallStaticVoidMethod(vm_class, dump_mid);
    env->DeleteLocalRef(vm_class);
  }
} // namespace jni
