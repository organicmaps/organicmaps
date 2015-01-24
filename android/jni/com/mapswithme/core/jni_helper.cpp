#include "jni_helper.hpp"
#include "logging.hpp"

#include "../../../../../base/assert.hpp"


static JavaVM * g_jvm = 0;

extern JavaVM * GetJVM()
{
  return g_jvm;
}

// @TODO remove after refactoring. Needed for NVidia code
void InitNVEvent(JavaVM * jvm);

extern "C"
{
/// fix for stack unwinding problem during exception handling on Android 2.1
/// http://code.google.com/p/android/issues/detail?id=20176
#ifdef __arm__

  typedef long unsigned int *_Unwind_Ptr;

  /* Stubbed out in libdl and defined in the dynamic linker.
   * Same semantics as __gnu_Unwind_Find_exidx().
   */
  _Unwind_Ptr dl_unwind_find_exidx(_Unwind_Ptr pc, int *pcount);
  _Unwind_Ptr __gnu_Unwind_Find_exidx(_Unwind_Ptr pc, int *pcount)
  {
    return dl_unwind_find_exidx(pc, pcount);
  }

  static void* g_func_ptr;

#endif // __arm__

  JNIEXPORT jint JNICALL
  JNI_OnLoad(JavaVM * jvm, void *)
  {
/// fix for stack unwinding problem during exception handling on Android 2.1
/// http://code.google.com/p/android/issues/detail?id=20176
#ifdef __arm__
    // when i throw exception, linker maybe can't find __gnu_Unwind_Find_exidx(lazy binding issue??)
    // so I force to bind this symbol at shared object load time
    g_func_ptr = (void*)__gnu_Unwind_Find_exidx;
#endif // __arm__

    g_jvm = jvm;
    jni::InitSystemLog();
    jni::InitAssertLog();
    // @TODO remove line below after refactoring
    InitNVEvent(jvm);
    return JNI_VERSION_1_6;
  }

  JNIEXPORT void JNICALL
  JNI_OnUnload(JavaVM *, void *)
  {
    g_jvm = 0;
  }
} // extern "C"


namespace jni
{
  jmethodID GetJavaMethodID(JNIEnv * env, jobject obj, char const * fn, char const * sig)
  {
    ASSERT(env, ("JNIEnv can't be 0"));
    ASSERT(obj, ("jobject can't be 0"));

    jclass cls = env->GetObjectClass(obj);
    ASSERT(cls, ("Can't get class: ", DescribeException()));

    jmethodID mid = env->GetMethodID(cls, fn, sig);
    ASSERT(mid, ("Can't get methodID", fn, sig, DescribeException()));
    return mid;
  }

  /*
  jobject CreateJavaObject(JNIEnv * env, char const * klassName, char const * sig, ...)
  {
    jclass klass = env->FindClass(klassName);
    ASSERT(klass, ("Can't find java class", klassName));

    jmethodID methodId = env->GetMethodID(klass, "<init>", sig);
    ASSERT(methodId, ("Can't find java constructor", sig));

    va_list args;
    va_start(args, sig);
    jobject res = env->NewObject(klass, methodId, args);
    ASSERT(res, ());
    va_end(args);

    return res;
  }
  */

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

} // namespace jni
