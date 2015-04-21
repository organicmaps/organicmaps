#include "mock.hpp"

#include "android/jni/com/mapswithme/core/logging.hpp"

#include "coding/file_writer.hpp"

#include "platform/file_logging.hpp"
#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/string.hpp"

using namespace my;

// @todo(vbykoianko) Probably it's worth thinking about output of the function to make the result of
// the tests more readable.
// @todo(vbykoianko) It's necessary display the test name in the android log.
static void AndroidLogMessage(LogLevel l, SrcPoint const & src, string const & s)
{
  android_LogPriority pr = ANDROID_LOG_SILENT;

  switch (l)
  {
  case LINFO: pr = ANDROID_LOG_INFO; break;
  case LDEBUG: pr = ANDROID_LOG_DEBUG; break;
  case LWARNING: pr = ANDROID_LOG_WARN; break;
  case LERROR: pr = ANDROID_LOG_ERROR; break;
  case LCRITICAL: pr = ANDROID_LOG_FATAL; break;
  }

  string const out = DebugPrint(src) + " " + s;
  __android_log_write(pr, " MapsMeTests ", out.c_str());
}

static void AndroidAssertMessage(SrcPoint const & src, string const & s)
{
#if defined(MWM_LOG_TO_FILE)
  AndroidLogToFile(LERROR, src, s);
#else
  AndroidLogMessage(LERROR, src, s);
#endif

#ifdef DEBUG
    assert(false);
#else
    MYTHROW(RootException, (s));
#endif
}

static void InitSystemLog()
{
#if defined(MWM_LOG_TO_FILE)
  SetLogMessageFn(&AndroidLogToFile);
#else
  SetLogMessageFn(&AndroidLogMessage);
#endif
}

static void InitAssertLog()
{
  SetAssertFunction(&AndroidAssertMessage);
}

namespace
{
  JavaVM * javaVM = nullptr;
}

namespace android_tests
{
  class MainThreadScopeGuard
  {
    JavaVM * m_Vm;
    JNIEnv * m_Env;
  public:
    MainThreadScopeGuard(JavaVM * vm) : m_Vm(vm), m_Env(nullptr) 
    { 
      assert(vm);
      m_Vm->AttachCurrentThread(&m_Env, NULL); 
    }
    ~MainThreadScopeGuard() { m_Vm->DetachCurrentThread(); }
    JNIEnv * GetEnv() const { return m_Env; }
  };

  static bool CheckExceptions(JNIEnv * env)
  {
    jthrowable exception = env->ExceptionOccurred();
    if (exception) 
    {
      env->ExceptionDescribe();
      env->ExceptionClear();
      return true;
    }
    return false;
  }

  static string GetApkPath(ANativeActivity * activity, JNIEnv * env)
  {
    ASSERT(activity, ());
    ASSERT(env, ());

    jclass const clazz = env->GetObjectClass(activity->clazz);
    ASSERT(clazz, ());
    jmethodID const methodID = env->GetMethodID(clazz, "getPackageCodePath", "()Ljava/lang/String;");
    ASSERT(methodID, ());
    jobject const result = env->CallObjectMethod(activity->clazz, methodID);
    ASSERT(result, ());
    ASSERT(!CheckExceptions(env), ());
    jboolean isCopy;
    std::string const res = env->GetStringUTFChars((jstring)result, &isCopy);
    return res;
  }

  static string GetSdcardPath(ANativeActivity * activity, JNIEnv * env)
  {
    ASSERT(activity, ());
    ASSERT(env, ());

    jclass const classEnvironment = env->FindClass("android/os/Environment");
    ASSERT(classEnvironment, ());
    jmethodID const methodIDgetExternalStorageDirectory = env->GetStaticMethodID(classEnvironment, "getExternalStorageDirectory", "()Ljava/io/File;");
    ASSERT(methodIDgetExternalStorageDirectory, ());
    jobject const objectFile = env->CallStaticObjectMethod(classEnvironment, methodIDgetExternalStorageDirectory);
    ASSERT(objectFile, ());
    ASSERT(!CheckExceptions(env), ());

    jclass const classFile = env->GetObjectClass(objectFile);
    ASSERT(classFile, ());
    jmethodID const methodIDgetAbsolutePath = env->GetMethodID(classFile, "getAbsolutePath", "()Ljava/lang/String;");
    ASSERT(methodIDgetAbsolutePath, ());
    jstring const stringPath = (jstring) env->CallObjectMethod(objectFile, methodIDgetAbsolutePath);
    ASSERT(stringPath, ());
    ASSERT(!CheckExceptions(env), ());

    jboolean isCopy;
    std::string const res = env->GetStringUTFChars((jstring)stringPath, &isCopy);
    return res;
  }

  static string GetPackageName(ANativeActivity * activity, JNIEnv * env)
  {
    ASSERT(activity, ());
    ASSERT(env, ());

    jclass const clazz = env->GetObjectClass(activity->clazz);
    ASSERT(clazz, ());
    jmethodID const methodID = env->GetMethodID(clazz, "getPackageName", "()Ljava/lang/String;");
    ASSERT(methodID, ());
    jobject const result = env->CallObjectMethod(activity->clazz, methodID);
    ASSERT(result, ());
    ASSERT(!CheckExceptions(env), ());
    jboolean isCopy;
    std::string const res = env->GetStringUTFChars((jstring)result, &isCopy);
    return res;
  }

  class Platform : public ::Platform
  {
  public:
    void Initialize(ANativeActivity * activity, JNIEnv * env)
    {
      LOG(LINFO, ("Platform::Initialize()"));
      string apkPath = android_tests::GetApkPath(activity, env);
      LOG(LINFO, ("Apk path FromJNI: ", apkPath.c_str()));

      string sdcardPath = android_tests::GetSdcardPath(activity, env);
      LOG(LINFO, ("Sdcard path FromJNI: ", sdcardPath.c_str()));

      string packageName = android_tests::GetPackageName(activity, env);
      LOG(LINFO, ("Package name FromJNI: ", packageName.c_str()));

      m_writableDir = sdcardPath + "/MapsWithMe/";
      m_resourcesDir = apkPath;
      
      m_tmpDir = sdcardPath + "/Android/data/" + packageName + "/cache/";
      m_settingsDir = m_writableDir; 
      m_androidDefResScope = "rfw";
    }

    void OnExternalStorageStatusChanged(bool isAvailable) 
    {
      LOG(LWARNING, ("OnExternalStorageStatusChanged() is not implemented."));
    }

    /// get storage path without ending "/MapsWithMe/"
    string GetStoragePathPrefix() const
    {
      return string("/sdcard");
    }
    /// assign storage path (should contain ending "/MapsWithMe/")
    void SetStoragePath(string const & path) {}

    bool HasAvailableSpaceForWriting(uint64_t size) const{ return true; }

    static void RunOnGuiThreadImpl(TFunctor const & fn, bool blocking = false)
    {
      ASSERT(false, ());
    }

    static Platform & Instance()
    {
      static Platform platform;
      return platform;
    }
  };
}

Platform & GetPlatform()
{
  return android_tests::Platform::Instance();
}

string GetAndroidSystemLanguage()
{
  return "en_US";
}

void AndroidThreadAttachToJVM()
{
  LOG(LWARNING, ("AndroidThreadAttachToJVM() is not implemented."));
}
void AndroidThreadDetachFromJVM()
{
  LOG(LWARNING, ("AndroidThreadDetachFromJVM() is not implemented."));
}

void Platform::RunOnGuiThread(TFunctor const & fn)
{
  LOG(LWARNING, ("Platform::RunOnGuiThread() is not implemented."));
}

JavaVM * GetJVM() 
{ 
  LOG(LWARNING, ("GetJVM() returns nullptr."));
  return nullptr; 
}

void Initialize(android_app * state)
{
  LOGI("Indexperf. Initialize()");
  assert(state);
  ANativeActivity * activity = state->activity;
  assert(activity);
  android_tests::MainThreadScopeGuard mainThreadScopeGuard(activity->vm);
  JNIEnv * env = mainThreadScopeGuard.GetEnv();
  assert(env);

  javaVM = activity->vm;

  InitSystemLog();
  InitAssertLog();
  android_tests::Platform::Instance().Initialize(activity, env);
  LOG(LINFO, ("Indexperf. Initialization finished...."));
}

