#include "jni_helper.hpp"
#include "logging.hpp"
#include "ScopedLocalRef.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"
#include "std/vector.hpp"

static JavaVM * g_jvm = 0;
extern JavaVM * GetJVM()
{
  return g_jvm;
}

// caching is necessary to create class from native threads
jclass g_mapObjectClazz;
jclass g_bookmarkClazz;

extern "C"
{
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM * jvm, void *)
{
  g_jvm = jvm;
  jni::InitSystemLog();
  jni::InitAssertLog();

  JNIEnv * env = jni::GetEnv();
  g_mapObjectClazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/bookmarks/data/MapObject");
  g_bookmarkClazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/bookmarks/data/Bookmark");

  return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM *, void *)
{
  g_jvm = 0;
  JNIEnv * env = jni::GetEnv();
  env->DeleteGlobalRef(g_mapObjectClazz);
  env->DeleteGlobalRef(g_bookmarkClazz);
}
} // extern "C"

namespace jni
{
JNIEnv * GetEnv()
{
  JNIEnv * env;
  if (JNI_OK != g_jvm->GetEnv((void **)&env, JNI_VERSION_1_6))
    MYTHROW(RootException, ("Can't get JNIEnv. Was thread attached to JVM?"));

  return env;
}

JavaVM * GetJVM()
{
  ASSERT(g_jvm, ("JVM is not initialized"));
  return g_jvm;
}

jmethodID GetMethodID(JNIEnv * env, jobject obj, char const * name, char const * signature)
{
  TScopedLocalClassRef clazz(env, env->GetObjectClass(obj));
  ASSERT(clazz.get(), ("Can't get class: ", DescribeException()));

  jmethodID mid = env->GetMethodID(clazz.get(), name, signature);
  ASSERT(mid, ("Can't get method ID", name, signature, DescribeException()));
  return mid;
}

jmethodID GetStaticMethodID(JNIEnv * env, jclass clazz, char const * name, char const * signature)
{
  jmethodID mid = env->GetStaticMethodID(clazz, name, signature);
  ASSERT(mid, ("Can't get static method ID", name, signature, DescribeException()));
  return mid;
}

jmethodID GetConstructorID(JNIEnv * env, jclass clazz, char const * signature)
{
  jmethodID const ctorID = env->GetMethodID(clazz, "<init>", signature);
  ASSERT(ctorID, (DescribeException()));
  return ctorID;
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

jobjectArray ToJavaStringArray(JNIEnv * env, vector<string> const & src)
{
  return ToJavaArray(env, GetStringClass(env), src,
                     [](JNIEnv * env, string const & item)
                     {
                       return ToJavaString(env, item.c_str());
                     });
}

jclass GetStringClass(JNIEnv * env)
{
  return env->FindClass(GetStringClassName());
}

char const * GetStringClassName()
{
  return "java/lang/String";
}

struct global_ref_deleter
{
  void operator()(jobject * ref)
  {
    GetEnv()->DeleteGlobalRef(*ref);
    delete ref;
  }
};

shared_ptr<jobject> make_global_ref(jobject obj)
{
  jobject * ref = new jobject;
  *ref = GetEnv()->NewGlobalRef(obj);
  return shared_ptr<jobject>(ref, global_ref_deleter());
}

string DescribeException()
{
  JNIEnv * env = GetEnv();

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
  jmethodID methodID = GetConstructorID(env, klass, "(DD)V");

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
  jmethodID methodID = GetConstructorID(env, klass, "(II)V");

  return env->NewObject(klass, methodID,
                        static_cast<jint>(point.x),
                        static_cast<jint>(point.y));
}

// This util method dumps content of local and global reference jni tables to logcat for debug and testing purposes
void DumpDalvikReferenceTables()
{
  JNIEnv * env = GetEnv();
  jclass vm_class = env->FindClass("dalvik/system/VMDebug");
  jmethodID dump_mid = env->GetStaticMethodID(vm_class, "dumpReferenceTables", "()V");
  env->CallStaticVoidMethod(vm_class, dump_mid);
  env->DeleteLocalRef(vm_class);
}
}  // namespace jni
