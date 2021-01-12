#include "com/mapswithme/platform/Platform.hpp"
#include "com/mapswithme/platform/GuiThread.hpp"

#include "com/mapswithme/core/jni_helper.hpp"

#include "com/mapswithme/util/NetworkPolicy.hpp"

#include "platform/network_policy.hpp"
#include "platform/settings.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <memory>
#include <utility>

#include <sys/system_properties.h>

std::string Platform::UniqueClientId() const
{
  JNIEnv * env = jni::GetEnv();
  static jmethodID const getInstallationId = jni::GetStaticMethodID(env, g_utilsClazz, "getInstallationId",
                                                                    "(Landroid/content/Context;)"
                                                                    "Ljava/lang/String;");
  jobject context = android::Platform::Instance().GetContext();
  static jstring const installationId
    = static_cast<jstring>(env->CallStaticObjectMethod(g_utilsClazz, getInstallationId, context));
  static std::string const result = jni::ToNativeString(env, installationId);
  return result;
}

std::string Platform::AdvertisingId() const
{
  JNIEnv *env = jni::GetEnv();
  static jmethodID const getAdvertisingId = jni::GetStaticMethodID(env, g_utilsClazz,
                                                                   "getAdvertisingId",
                                                                   "(Landroid/content/Context;)"
                                                                   "Ljava/lang/String;");
  jobject context = android::Platform::Instance().GetContext();
  jni::TScopedLocalRef adIdRef(env, env->CallStaticObjectMethod(g_utilsClazz, getAdvertisingId,
                                                                context));
  if (adIdRef.get() == nullptr)
    return {};

  return jni::ToNativeString(env, static_cast<jstring>(adIdRef.get()));
}

std::string Platform::MacAddress(bool md5Decoded) const
{
  JNIEnv * env = jni::GetEnv();
  static jmethodID const getMacAddressMethod = jni::GetStaticMethodID(env, g_utilsClazz, "getMacAddress",
                                                                      "(Landroid/content/Context;)"
                                                                      "(Z)Ljava/lang/String;");
  jobject context = android::Platform::Instance().GetContext();
  auto const macAddr = static_cast<jstring>(env->CallStaticObjectMethod(g_utilsClazz,
                                                                        getMacAddressMethod,
                                                                        context,
                                                                        static_cast<jboolean>(md5Decoded)));
  return jni::ToNativeString(env, macAddr);
}

std::string Platform::GetMemoryInfo() const
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return std::string();

  static std::shared_ptr<jobject> classMemLogging = jni::make_global_ref(env->FindClass("com/mapswithme/util/log/MemLogging"));
  ASSERT(classMemLogging, ());

  jobject context = android::Platform::Instance().GetContext();
  static jmethodID const getMemoryInfoId
    = jni::GetStaticMethodID(env,
                             static_cast<jclass>(*classMemLogging),
                             "getMemoryInfo",
                             "(Landroid/content/Context;)Ljava/lang/String;");
  jstring const memInfoString = static_cast<jstring>(env->CallStaticObjectMethod(
    static_cast<jclass>(*classMemLogging), getMemoryInfoId, context));
  ASSERT(memInfoString, ());

  return jni::ToNativeString(env, memInfoString);
}

std::string Platform::DeviceName() const
{
  JNIEnv * env = jni::GetEnv();
  static jmethodID const getDeviceNameId = jni::GetStaticMethodID(env, g_utilsClazz, "getDeviceName",
                                                                  "()Ljava/lang/String;");
  auto const deviceName = static_cast<jstring>(env->CallStaticObjectMethod(g_utilsClazz,
                                                                           getDeviceNameId));
  return jni::ToNativeString(env, deviceName);
}

std::string Platform::DeviceModel() const
{
  JNIEnv * env = jni::GetEnv();
  static jmethodID const getDeviceModelId = jni::GetStaticMethodID(env, g_utilsClazz, "getDeviceModel",
                                                                  "()Ljava/lang/String;");
  auto const deviceModel = static_cast<jstring>(env->CallStaticObjectMethod(g_utilsClazz,
                                                                            getDeviceModelId));
  return jni::ToNativeString(env, deviceModel);
}

Platform::EConnectionType Platform::ConnectionStatus()
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return EConnectionType::CONNECTION_NONE;

  static std::shared_ptr<jobject> clazzConnectionState = jni::make_global_ref(env->FindClass("com/mapswithme/util/ConnectionState"));
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
      jni::GetStaticMethodID(env, clazzBatteryState, "getChargingStatus", "(Landroid/content/Context;)I");
  jobject context = android::Platform::Instance().GetContext();
  return static_cast<Platform::ChargingStatus>(
      env->CallStaticIntMethod(clazzBatteryState, getChargingMethodId, context));
}

uint8_t Platform::GetBatteryLevel()
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return 100;

  static auto const clazzBatteryState =
      jni::GetGlobalClassRef(env, "com/mapswithme/util/BatteryState");
  ASSERT(clazzBatteryState, ());

  static auto const getLevelMethodId =
      jni::GetStaticMethodID(env, clazzBatteryState, "getLevel", "(Landroid/content/Context;)I");
  jobject context = android::Platform::Instance().GetContext();
  return static_cast<uint8_t>(env->CallStaticIntMethod(clazzBatteryState, getLevelMethodId, context));
}

namespace platform
{
platform::NetworkPolicy GetCurrentNetworkPolicy()
{
  JNIEnv *env = jni::GetEnv();
  return platform::NetworkPolicy(network_policy::GetCurrentNetworkUsageStatus(env));
}
}

namespace android
{
Platform::Platform()
{
  auto env = jni::GetEnv();
  static auto const getSettingsPathId =
      jni::GetStaticMethodID(env, g_storageUtilsClazz, "getSettingsPath", "()Ljava/lang/String;");

  auto const settingsDir =
      static_cast<jstring>(env->CallStaticObjectMethod(g_storageUtilsClazz, getSettingsPathId));

  SetSettingsDir(jni::ToNativeString(env, settingsDir));
}

void Platform::Initialize(JNIEnv * env, jobject functorProcessObject, jstring apkPath,
                          jstring storagePath, jstring privatePath, jstring tmpPath,
                          jstring obbGooglePath, jstring flavorName, jstring buildType,
                          bool isTablet)
{
  m_functorProcessObject = env->NewGlobalRef(functorProcessObject);
  jclass const functorProcessClass = env->GetObjectClass(functorProcessObject);
  m_sendPushWooshTagsMethod = env->GetMethodID(functorProcessClass, "sendPushWooshTags",
      "(Ljava/lang/String;[Ljava/lang/String;)V");
  m_sendAppsFlyerTagsMethod = env->GetMethodID(functorProcessClass, "sendAppsFlyerTags",
      "(Ljava/lang/String;[Lcom/mapswithme/util/KeyValue;)V");

  m_guiThread = std::make_unique<GuiThread>(m_functorProcessObject);

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
  m_privateDir = jni::ToNativeString(env, privatePath);
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

void Platform::SendMarketingEvent(std::string const & tag,
                                  std::map<std::string, std::string> const & params)
{
  JNIEnv * env = jni::GetEnv();

  ASSERT(m_functorProcessObject, ());
  ASSERT(m_sendAppsFlyerTagsMethod, ());
  env->CallVoidMethod(m_functorProcessObject, m_sendAppsFlyerTagsMethod,
                      jni::TScopedLocalRef(env, jni::ToJavaString(env, tag)).get(),
                      jni::TScopedLocalObjectArrayRef(env, jni::ToKeyValueArray(env, params)).get());
}

void Platform::AndroidSecureStorage::Init(JNIEnv * env)
{
  if (m_secureStorageClass != nullptr)
    return;

  m_secureStorageClass = jni::GetGlobalClassRef(env, "com/mapswithme/util/SecureStorage");
  ASSERT(m_secureStorageClass, ());
}

void Platform::AndroidSecureStorage::Save(std::string const & key, std::string const & value)
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return;

  Init(env);

  static jmethodID const saveMethodId =
    jni::GetStaticMethodID(env, m_secureStorageClass, "save",
                           "(Landroid/content/Context;Ljava/lang/String;"
                           "Ljava/lang/String;)V");
  jobject context = android::Platform::Instance().GetContext();
  env->CallStaticVoidMethod(m_secureStorageClass, saveMethodId,
                            context,
                            jni::TScopedLocalRef(env, jni::ToJavaString(env, key)).get(),
                            jni::TScopedLocalRef(env, jni::ToJavaString(env, value)).get());
}

bool Platform::AndroidSecureStorage::Load(std::string const & key, std::string & value)
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return false;

  Init(env);

  static jmethodID const loadMethodId =
    jni::GetStaticMethodID(env, m_secureStorageClass, "load",
                           "(Landroid/content/Context;Ljava/lang/String;)"
                           "Ljava/lang/String;");
  jobject context = android::Platform::Instance().GetContext();
  auto const resultString = static_cast<jstring>(env->CallStaticObjectMethod(m_secureStorageClass,
    loadMethodId, context,
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

  Init(env);

  static jmethodID const removeMethodId =
    jni::GetStaticMethodID(env, m_secureStorageClass, "remove",
                           "(Landroid/content/Context;Ljava/lang/String;)V");
  jobject context = android::Platform::Instance().GetContext();
  env->CallStaticVoidMethod(m_secureStorageClass, removeMethodId, context,
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
