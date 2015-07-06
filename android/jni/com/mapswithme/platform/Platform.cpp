#include "Platform.hpp"

#include "../core/jni_helper.hpp"

#include "../../../nv_event/nv_event.hpp"

#include "platform/settings.hpp"

#include "base/logging.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"


string Platform::UniqueClientId() const
{
  string res;
  if (!Settings::Get("UniqueClientID", res))
  {
    JNIEnv * env = jni::GetEnv();
    if (!env)
      return string();

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

string Platform::GetMemoryInfo() const
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return string();

  static shared_ptr<jobject> classMemLogging = jni::make_global_ref(env->FindClass("com/mapswithme/util/log/MemLogging"));
  ASSERT(classMemLogging, ());

  static jmethodID const getMemoryInfoId = env->GetStaticMethodID(static_cast<jclass>(*classMemLogging.get()), "getMemoryInfo", "()Ljava/lang/String;");
  ASSERT(getMemoryInfoId, ());

  jstring const memInfoString = (jstring)env->CallStaticObjectMethod(static_cast<jclass>(*classMemLogging.get()), getMemoryInfoId);
  ASSERT(memInfoString, ());

  return jni::ToNativeString(env, memInfoString);
}

void Platform::RunOnGuiThread(TFunctor const & fn)
{
  android::Platform::RunOnGuiThreadImpl(fn);
}

Platform::EConnectionType Platform::ConnectionStatus()
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return EConnectionType::CONNECTION_NONE;

  static shared_ptr<jobject> clazzConnectionState = jni::make_global_ref(env->FindClass("com/mapswithme/util/ConnectionState"));
  ASSERT(clazzConnectionState, ());

  static jmethodID const getConnectionMethodId = env->GetStaticMethodID(static_cast<jclass>(*clazzConnectionState.get()), "getConnectionState", "()B");
  ASSERT(getConnectionMethodId, ());

  return static_cast<Platform::EConnectionType>(env->CallStaticByteMethod(static_cast<jclass>(*clazzConnectionState.get()), getConnectionMethodId));
}

namespace android
{
  void Platform::Initialize(JNIEnv * env,
                            jstring apkPath, jstring storagePath,
                            jstring tmpPath, jstring obbGooglePath,
                            jstring flavorName, jstring buildType,
                            bool isYota, bool isTablet)
  {
    string const flavor = jni::ToNativeString(env, flavorName);
    string const build = jni::ToNativeString(env, buildType);
    LOG(LINFO, ("Flavor name:", flavor));
    LOG(LINFO, ("Build type name:", build));

    if (build == "beta" || build == "debug")
      m_androidDefResScope = "fwr";
    else if (flavor.find("google") == 0)
      m_androidDefResScope = "ferw";
    else if (flavor.find("amazon") == 0 || flavor.find("samsung") == 0)
      m_androidDefResScope = "frw";
    else
      m_androidDefResScope = "fwr";

    m_isTablet = isTablet;

    m_resourcesDir = jni::ToNativeString(env, apkPath);

    // Settings file should be in a one place always (default external storage).
    // It stores path to the current maps storage.
    m_settingsDir = jni::ToNativeString(env, storagePath);

    // @TODO it's a bug when user had all his maps on SD but when space is low,
    // he loses access to all downloaded maps. We should display warnings in these cases in UI.
    if (!Settings::Get("StoragePath", m_writableDir) || !HasAvailableSpaceForWriting(1024))
    {
      // If no saved storage path or the storage is unavailable
      // (disconnected from the last session), assign writable
      // path to the default external storage.
      m_writableDir = m_settingsDir;
    }

    m_tmpDir = jni::ToNativeString(env, tmpPath);

    string const obbPath = jni::ToNativeString(env, obbGooglePath);
    Platform::FilesList files;
    GetFilesByExt(obbPath, ".obb", files);
    m_extResFiles.clear();
    for (size_t i = 0; i < files.size(); ++i)
      m_extResFiles.push_back(obbPath + files[i]);

    LOG(LINFO, ("Apk path = ", m_resourcesDir));
    LOG(LINFO, ("Writable path = ", m_writableDir));
    LOG(LINFO, ("Temporary path = ", m_tmpDir));
    LOG(LINFO, ("Settings path = ", m_settingsDir));
    LOG(LINFO, ("OBB Google path = ", obbPath));
    LOG(LINFO, ("OBB Google files = ", files));

    // IMPORTANT: This method SHOULD be called from UI thread to cache static jni ID-s inside.
    (void) ConnectionStatus();
  }

  void Platform::OnExternalStorageStatusChanged(bool isAvailable)
  {
  }

  string Platform::GetStoragePathPrefix() const
  {
    size_t const count = m_writableDir.size();
    ASSERT_GREATER ( count, 2, () );

    size_t const i = m_writableDir.find_last_of('/', count-2);
    ASSERT_GREATER ( i, 0, () );

    return m_writableDir.substr(0, i);
  }

  void Platform::SetStoragePath(string const & path)
  {
    m_writableDir = path;
    Settings::Set("StoragePath", m_writableDir);
  }

  bool Platform::HasAvailableSpaceForWriting(uint64_t size) const
  {
    return (GetWritableStorageStatus(size) == ::Platform::STORAGE_OK);
  }

  Platform & Platform::Instance()
  {
    static Platform platform;
    return platform;
  }

  void Platform::RunOnGuiThreadImpl(TFunctor const & fn, bool blocking)
  {
    postMWMEvent(new TFunctor(fn), blocking);
  }
}

Platform & GetPlatform()
{
  return android::Platform::Instance();
}
