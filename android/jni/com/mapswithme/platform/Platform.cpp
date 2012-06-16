#include "Platform.hpp"

#include "../core/jni_helper.hpp"

#include "../../../../../platform/settings.hpp"
#include "../../../../../base/logging.hpp"

#include "../../../../../std/algorithm.hpp"
#include "../../../../../std/cmath.hpp"

class Platform::PlatformImpl
{
public:

  PlatformImpl() : m_preCachingDepth(3)
  {}

  size_t m_preCachingDepth;
};

int Platform::PreCachingDepth() const
{
  return m_impl->m_preCachingDepth;
}

string Platform::UniqueClientId() const
{
  string res;
  if (!Settings::Get("UniqueClientID", res))
  {
    JNIEnv * env = jni::GetEnv();
    if (!env)
    {
      LOG(LWARNING, ("Can't get JNIEnv"));
      return "";
    }

    jclass uuidClass = env->FindClass("java/util/UUID");
    ASSERT(uuidClass, ("Can't find java class java/util/UUID"));

    jmethodID randomUUIDId = env->GetStaticMethodID(uuidClass, "randomUUID", "()Ljava/util/UUID;");
    ASSERT(randomUUIDId, ("Can't find static java/util/UUID.randomUUIDId() method"));

    jobject uuidInstance = env->CallStaticObjectMethod(uuidClass, randomUUIDId);
    ASSERT(uuidInstance, ("UUID.randomUUID() returned NULL"));

    jmethodID toStringId = env->GetMethodID(uuidClass, "toString", "()Ljava/lang/String;");
    ASSERT(toStringId, ("Can't find java/util/UUID.toString() method"));

    jstring uuidString = (jstring)env->CallObjectMethod(uuidInstance, toStringId);
    ASSERT(uuidString, ("UUID.toString() returned NULL"));

    char const * uuidUtf8 = env->GetStringUTFChars(uuidString, 0);

    if (uuidUtf8 != 0)
    {
      res = uuidUtf8;
      env->ReleaseStringUTFChars(uuidString, uuidUtf8);
    }

    res = HashUniqueID(res);

    Settings::Set("UniqueClientID", res);
  }

  return res;
}

namespace android
{
  Platform::~Platform()
  {
    delete m_impl;
  }

  void Platform::Initialize(JNIEnv * env,
                            jstring apkPath,
                            jstring storagePath,
                            jstring tmpPath,
                            jstring extTmpPath,
                            jstring settingsPath)
  {
    if (m_impl)
      delete m_impl;

    m_impl = new PlatformImpl();

    m_resourcesDir = jni::ToNativeString(env, apkPath);
    m_writableDir = jni::ToNativeString(env, storagePath);
    m_settingsDir = jni::ToNativeString(env, settingsPath);

    m_localTmpPath = jni::ToNativeString(env, tmpPath);
    m_externalTmpPath = jni::ToNativeString(env, extTmpPath);
    // By default use external temporary folder
    m_tmpDir = m_externalTmpPath;

    LOG(LDEBUG, ("Apk path = ", m_resourcesDir));
    LOG(LDEBUG, ("Writable path = ", m_writableDir));
    LOG(LDEBUG, ("Local tmp path = ", m_localTmpPath));
    LOG(LDEBUG, ("External tmp path = ", m_externalTmpPath));
    LOG(LDEBUG, ("Settings path = ", m_settingsDir));
  }

  void Platform::OnExternalStorageStatusChanged(bool isAvailable)
  {
    if (isAvailable)
      m_tmpDir = m_externalTmpPath;
    else
      m_tmpDir = m_localTmpPath;
  }

  Platform & Platform::Instance()
  {
    static Platform platform;
    return platform;
  }
}

Platform & GetPlatform()
{
  return android::Platform::Instance();
}
