#include "android/jni/com/mapswithme/platform/Platform.hpp"
#include "android/jni/com/mapswithme/platform/GuiThread.hpp"

#include "android/jni/com/mapswithme/core/jni_helper.hpp"

#include "platform/settings.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <utility>

#include <sys/system_properties.h>

std::string Platform::UniqueClientId() const
{
  JNIEnv * env = jni::GetEnv();
  static jmethodID const getInstallationId = jni::GetStaticMethodID(env, g_utilsClazz, "getInstallationId",
                                                                    "()Ljava/lang/String;");
  static jstring const installationId = (jstring)env->CallStaticObjectMethod(g_utilsClazz, getInstallationId);
  static std::string const result = jni::ToNativeString(env, installationId);
  return result;
}

std::string Platform::GetMemoryInfo() const
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return std::string();

  static shared_ptr<jobject> classMemLogging = jni::make_global_ref(env->FindClass("com/mapswithme/util/log/MemLogging"));
  ASSERT(classMemLogging, ());

  static jmethodID const getMemoryInfoId = jni::GetStaticMethodID(env, static_cast<jclass>(*classMemLogging), "getMemoryInfo", "()Ljava/lang/String;");
  jstring const memInfoString = (jstring)env->CallStaticObjectMethod(static_cast<jclass>(*classMemLogging), getMemoryInfoId);
  ASSERT(memInfoString, ());

  return jni::ToNativeString(env, memInfoString);
}

void Platform::RunOnGuiThread(base::TaskLoop::Task && task)
{
  android::Platform::Instance().RunOnGuiThread(std::move(task));
}

void Platform::RunOnGuiThread(base::TaskLoop::Task const & task)
{
  android::Platform::Instance().RunOnGuiThread(task);
}

Platform::EConnectionType Platform::ConnectionStatus()
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return EConnectionType::CONNECTION_NONE;

  static shared_ptr<jobject> clazzConnectionState = jni::make_global_ref(env->FindClass("com/mapswithme/util/ConnectionState"));
  ASSERT(clazzConnectionState, ());

  static jmethodID const getConnectionMethodId = jni::GetStaticMethodID(env, static_cast<jclass>(*clazzConnectionState), "getConnectionState", "()B");
  return static_cast<Platform::EConnectionType>(env->CallStaticByteMethod(static_cast<jclass>(*clazzConnectionState), getConnectionMethodId));
}

Platform::ChargingStatus Platform::GetChargingStatus()
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return Platform::ChargingStatus::Unknown;

  static jclass const clazzBatteryState =
      jni::GetGlobalClassRef(env, "com/mapswithme/util/BatteryState");
  ASSERT(clazzBatteryState, ());

  static jmethodID const getChargingMethodId =
      jni::GetStaticMethodID(env, clazzBatteryState, "getChargingStatus", "()I");
  return static_cast<Platform::ChargingStatus>(
      env->CallStaticIntMethod(clazzBatteryState, getChargingMethodId));
}

void Platform::SetGuiThread(unique_ptr<base::TaskLoop> guiThread)
{
  android::Platform::Instance().SetGuiThread(move(guiThread));
}

namespace android
{
void Platform::Initialize(JNIEnv * env,
                          jobject functorProcessObject,
                          jstring apkPath, jstring storagePath,
                          jstring tmpPath, jstring obbGooglePath,
                          jstring flavorName, jstring buildType,
                          bool isTablet)
{
  m_functorProcessObject = env->NewGlobalRef(functorProcessObject);
  jclass const functorProcessClass = env->GetObjectClass(functorProcessObject);
  m_sendPushWooshTagsMethod = env->GetMethodID(functorProcessClass, "sendPushWooshTags", "(Ljava/lang/String;[Ljava/lang/String;)V");
  m_myTrackerTrackMethod = env->GetStaticMethodID(g_myTrackerClazz, "trackEvent", "(Ljava/lang/String;)V");

  m_secureStorage.Init();

  m_guiThread = my::make_unique<GuiThread>(m_functorProcessObject);

  std::string const flavor = jni::ToNativeString(env, flavorName);
  std::string const build = jni::ToNativeString(env, buildType);
  LOG(LINFO, ("Flavor name:", flavor));
  LOG(LINFO, ("Build type name:", build));

  if (build == "beta" || build == "debug")
    m_androidDefResScope = "fwr";
  else if (flavor.find("google") == 0)
    m_androidDefResScope = "ferw";
  else if (flavor.find("amazon") == 0 || flavor.find("samsung") == 0) // optimization to read World mwm-s faster
    m_androidDefResScope = "frw";
  else
    m_androidDefResScope = "fwr";

  m_isTablet = isTablet;
  m_resourcesDir = jni::ToNativeString(env, apkPath);
  m_tmpDir = jni::ToNativeString(env, tmpPath);
  m_writableDir = jni::ToNativeString(env, storagePath);

  std::string const obbPath = jni::ToNativeString(env, obbGooglePath);
  Platform::FilesList files;
  GetFilesByExt(obbPath, ".obb", files);
  m_extResFiles.clear();
  for (size_t i = 0; i < files.size(); ++i)
    m_extResFiles.push_back(obbPath + files[i]);

  LOG(LINFO, ("Apk path = ", m_resourcesDir));
  LOG(LINFO, ("Writable path = ", m_writableDir));
  LOG(LINFO, ("Temporary path = ", m_tmpDir));
  LOG(LINFO, ("OBB Google path = ", obbPath));
  LOG(LINFO, ("OBB Google files = ", files));

  // IMPORTANT: This method SHOULD be called from UI thread to cache static jni ID-s inside.
  (void) ConnectionStatus();
}

Platform::~Platform()
{
  JNIEnv * env = jni::GetEnv();

  if (m_functorProcessObject)
    env->DeleteGlobalRef(m_functorProcessObject);
}

void Platform::ProcessFunctor(jlong functionPointer)
{
  TFunctor * fn = reinterpret_cast<TFunctor *>(functionPointer);
  (*fn)();
  delete fn;
}

void Platform::OnExternalStorageStatusChanged(bool isAvailable)
{
}

std::string Platform::GetStoragePathPrefix() const
{
  size_t const count = m_writableDir.size();
  ASSERT_GREATER ( count, 2, () );

  size_t const i = m_writableDir.find_last_of('/', count-2);
  ASSERT_GREATER ( i, 0, () );

  return m_writableDir.substr(0, i);
}

void Platform::SetWritableDir(std::string const & dir)
{
  m_writableDir = dir;
  settings::Set("StoragePath", m_writableDir);
  LOG(LINFO, ("Writable path = ", m_writableDir));
}

void Platform::SetSettingsDir(std::string const & dir)
{
  m_settingsDir = dir;
  LOG(LINFO, ("Settings path = ", m_settingsDir));
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

void Platform::SendPushWooshTag(std::string const & tag, std::vector<std::string> const & values)
{
  ASSERT(m_functorProcessObject, ());
  ASSERT(m_sendPushWooshTagsMethod, ());

  if (values.empty())
    return;

  JNIEnv * env = jni::GetEnv();
  env->CallVoidMethod(m_functorProcessObject, m_sendPushWooshTagsMethod,
                      jni::TScopedLocalRef(env, jni::ToJavaString(env, tag)).get(),
                      jni::TScopedLocalObjectArrayRef(env, jni::ToJavaStringArray(env, values)).get());
}

void Platform::SendMarketingEvent(std::string const & tag, std::map<std::string, std::string> const & params)
{
  ASSERT(m_myTrackerTrackMethod, ());

  JNIEnv * env = jni::GetEnv();
  std::string eventData = tag;
  for (auto const & item : params)
    eventData.append("_" + item.first + "_" + item.second);

  env->CallStaticVoidMethod(g_myTrackerClazz, m_myTrackerTrackMethod,
                            jni::TScopedLocalRef(env, jni::ToJavaString(env, eventData)).get());
}

void Platform::SetGuiThread(unique_ptr<base::TaskLoop> guiThread)
{
  m_guiThread = std::move(guiThread);
}

void Platform::AndroidSecureStorage::Init()
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return;

  m_secureStorageClass = jni::GetGlobalClassRef(env, "com/mapswithme/util/SecureStorage");
  ASSERT(m_secureStorageClass, ());
}

void Platform::AndroidSecureStorage::Save(std::string const & key, std::string const & value)
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return;

  static jmethodID const saveMethodId =
    jni::GetStaticMethodID(env, m_secureStorageClass, "save", "(Ljava/lang/String;Ljava/lang/String;)V");

  env->CallStaticVoidMethod(m_secureStorageClass, saveMethodId,
                            jni::TScopedLocalRef(env, jni::ToJavaString(env, key)).get(),
                            jni::TScopedLocalRef(env, jni::ToJavaString(env, value)).get());
}

bool Platform::AndroidSecureStorage::Load(std::string const & key, std::string & value)
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return false;

  static jmethodID const loadMethodId =
    jni::GetStaticMethodID(env, m_secureStorageClass, "load", "(Ljava/lang/String;)Ljava/lang/String;");

  auto const resultString = static_cast<jstring>(env->CallStaticObjectMethod(m_secureStorageClass, loadMethodId,
    jni::TScopedLocalRef(env, jni::ToJavaString(env, key)).get()));
  if (resultString == nullptr)
    return false;

  value = jni::ToNativeString(env, resultString);
  return true;
}

void Platform::AndroidSecureStorage::Remove(std::string const & key)
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return;

  static jmethodID const removeMethodId =
    jni::GetStaticMethodID(env, m_secureStorageClass, "remove", "(Ljava/lang/String;)V");

  env->CallStaticVoidMethod(m_secureStorageClass, removeMethodId,
                            jni::TScopedLocalRef(env, jni::ToJavaString(env, key)).get());
}

int GetAndroidSdkVersion()
{
  char osVersion[PROP_VALUE_MAX + 1];
  if (__system_property_get("ro.build.version.sdk", osVersion) == 0)
    return 0;

  int version;
  if (!strings::to_int(std::string(osVersion), version))
    version = 0;

  return version;
}
}  // namespace android

Platform & GetPlatform()
{
  return android::Platform::Instance();
}
