#include "Framework.hpp"

#include "../core/jni_helper.hpp"

#include "coding/internal/file_data.hpp"
#include "platform/mwm_version.hpp"
#include "storage/storage.hpp"

#include "std/bind.hpp"
#include "std/shared_ptr.hpp"

namespace data
{

using namespace storage;

enum ItemCategory : uint32_t
{
  NEAR_ME,
  DOWNLOADED,
  ALL,
};

jclass g_listClass;
jmethodID g_listAddMethod;
jclass g_countryItemClass;

Storage & GetStorage()
{
  return g_framework->Storage();
}

void PrepareClassRefs(JNIEnv * env)
{
  if (g_listClass)
    return;

  g_listClass = jni::GetGlobalClassRef(env, "java/util/List");
  g_listAddMethod = env->GetMethodID(g_listClass, "add", "(Ljava/lang/Object;)Z");
  g_countryItemClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/downloader/CountryItem");
}

string GetLocalizedName(TCountryId const & id)
{
  // TODO
  return id;
}

} // namespace data

extern "C"
{

using namespace storage;
using namespace data;


// static boolean nativeMoveFile(String oldFile, String newFile);
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_MapStorage_nativeMoveFile(JNIEnv * env, jclass clazz, jstring oldFile, jstring newFile)
{
  return my::RenameFileX(jni::ToNativeString(env, oldFile), jni::ToNativeString(env, newFile));
}

// static native boolean nativeIsLegacyMode();
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeIsLegacyMode(JNIEnv * env, jclass clazz)
{
  return g_framework->NeedMigrate();
}

// static void nativeMigrate();
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeMigrate(JNIEnv * env, jclass clazz)
{
  g_framework->Migrate();
}

// static int nativeGetDownloadedCount();
JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeGetDownloadedCount(JNIEnv * env, jclass clazz)
{
  return GetStorage().GetDownloadedFilesCount();
}

// static String nativeGetRootNode();
JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeGetRootNode(JNIEnv * env, jclass clazz)
{
  return jni::ToJavaString(env, Storage().GetRootId());
}

// static @Nullable UpdateInfo nativeGetUpdateInfo();
JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeGetUpdateInfo(JNIEnv * env, jclass clazz)
{
  static Storage::UpdateInfo info = { 0 };
  if (!GetStorage().GetUpdateInfo(info))
    return nullptr;

  static jclass const infoClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/downloader/UpdateInfo");
  ASSERT(infoClass, (jni::DescribeException()));
  static jmethodID const ctor = env->GetMethodID(infoClass, "<init>", "(II)V");
  ASSERT(ctor, (jni::DescribeException()));

  return env->NewObject(infoClass, ctor, info.m_numberOfMwmFilesToUpdate, info.m_totalUpdateSizeInBytes);
}

static void UpdateItem(JNIEnv * env, jobject item, NodeAttrs const & attrs)
{
  static jfieldID const countryItemFieldName = env->GetFieldID(g_countryItemClass, "name", "Ljava/lang/String;");
  static jfieldID const countryItemFieldSize = env->GetFieldID(g_countryItemClass, "size", "J");
  static jfieldID const countryItemFieldTotalSize = env->GetFieldID(g_countryItemClass, "totalSize", "J");
  static jfieldID const countryItemFieldChildCount = env->GetFieldID(g_countryItemClass, "childCount", "I");
  static jfieldID const countryItemFieldTotalChildCount = env->GetFieldID(g_countryItemClass, "totalChildCount", "I");
  static jfieldID const countryItemFieldStatus = env->GetFieldID(g_countryItemClass, "status", "I");
  static jfieldID const countryItemFieldErrorCode = env->GetFieldID(g_countryItemClass, "errorCode", "I");
  static jfieldID const countryItemFieldPresent = env->GetFieldID(g_countryItemClass, "present", "Z");

  // Localized name
  jstring name = jni::ToJavaString(env, attrs.m_nodeLocalName);
  env->SetObjectField(item, countryItemFieldName, name);
  env->DeleteLocalRef(name);

  // Sizes
  env->SetLongField(item, countryItemFieldSize, attrs.m_localMwmSize);
  env->SetLongField(item, countryItemFieldTotalSize, attrs.m_mwmSize);

  // Child counts
  env->SetIntField(item, countryItemFieldChildCount, attrs.m_localMwmCounter);
  env->SetIntField(item, countryItemFieldTotalChildCount, attrs.m_mwmCounter);

  // Status and error code
  env->SetIntField(item, countryItemFieldStatus, static_cast<jint>(attrs.m_status));
  env->SetIntField(item, countryItemFieldErrorCode, static_cast<jint>(attrs.m_error));

  // Presence flag
  env->SetBooleanField(item, countryItemFieldPresent, attrs.m_present);
}

static void PutItemsToList(JNIEnv * env, jobject const list,  vector<TCountryId> const & children, TCountryId const & parent,
                           int category, function<void (jobject const)> const & callback)
{
  static jmethodID const countryItemCtor = env->GetMethodID(g_countryItemClass, "<init>", "(Ljava/lang/String;)V");
  static jfieldID const countryItemFieldCategory = env->GetFieldID(g_countryItemClass, "category", "I");
  static jfieldID const countryItemFieldParentId = env->GetFieldID(g_countryItemClass, "parentId", "Ljava/lang/String;");

  jstring parentId = jni::ToJavaString(env, parent);
  NodeAttrs attrs;

  for (TCountryId const & child : children)
  {
    GetStorage().GetNodeAttrs(child, attrs);

    jstring id = jni::ToJavaString(env, child);
    jobject item = env->NewObject(g_countryItemClass, countryItemCtor, id);
    env->SetIntField(item, countryItemFieldCategory, category);

    env->SetObjectField(item, countryItemFieldParentId, parentId);

    UpdateItem(env, item, attrs);

    // Let the caller do special processing
    callback(item);

    // Put to resulting list
    env->CallBooleanMethod(list, g_listAddMethod, item);

    // Drop local refs
    env->DeleteLocalRef(item);
    env->DeleteLocalRef(id);
  }
}

// static void nativeListItems(@Nullable String parent, List<CountryItem> result);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeListItems(JNIEnv * env, jclass clazz, jstring parent, jobject result)
{
  PrepareClassRefs(env);

  Storage const & storage = GetStorage();
  TCountryId const parentId = (parent ? jni::ToNativeString(env, parent) : storage.GetRootId());

  static jfieldID const countryItemFieldParentId = env->GetFieldID(g_countryItemClass, "parentId", "Ljava/lang/String;");
  static jfieldID const countryItemFieldParentName = env->GetFieldID(g_countryItemClass, "parentName", "Ljava/lang/String;");

  if (parent)
  {
    vector<TCountryId> children;
    storage.GetChildren(parentId, children);

    jstring parentName = jni::ToJavaString(env, GetLocalizedName(parentId));
    PutItemsToList(env, result, children, parentId, ItemCategory::ALL, [env, parent, parentName](jobject const item)
    {
      env->SetObjectField(item, countryItemFieldParentId, parent);
      env->SetObjectField(item, countryItemFieldParentName, parentName);
    });
  }
  else
  {
    // TODO (trashkalmar): Countries near me

    // Downloaded
    vector<TCountryId> children;
    storage.GetDownloadedChildren(parentId, children);
    PutItemsToList(env, result, children, parentId, ItemCategory::DOWNLOADED, [env](jobject const item)
    {

    });

    // All
    storage.GetChildren(parentId, children);
    PutItemsToList(env, result, children, parentId, ItemCategory::ALL, [env](jobject const item)
    {

    });

    //
  }
}

// static void nativeUpdateItem(CountryItem item);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeGetAttributes(JNIEnv * env, jclass clazz, jobject item)
{
  PrepareClassRefs(env);

  NodeAttrs attrs;
  static jfieldID countryItemFieldId = env->GetFieldID(g_countryItemClass, "id", "Ljava/lang/String;");
  jstring id = static_cast<jstring>(env->GetObjectField(item, countryItemFieldId));
  GetStorage().GetNodeAttrs(jni::ToNativeString(env, id), attrs);

  UpdateItem(env, item, attrs);
}

// static @Nullable String nativeFindCountry(double lat, double lon);
JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeFindCountry(JNIEnv * env, jclass clazz, jdouble lat, jdouble lon)
{
  return jni::ToJavaString(env, g_framework->NativeFramework()->CountryInfoGetter().GetRegionCountryId(MercatorBounds::FromLatLon(lat, lon)));
}

// static boolean nativeIsDownloading();
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeIsDownloading(JNIEnv * env, jclass clazz)
{
  return GetStorage().IsDownloadInProgress();
}

// static boolean nativeDownload(String root);
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeDownload(JNIEnv * env, jclass clazz, jstring root)
{
  return GetStorage().DownloadNode(jni::ToNativeString(env, root));
}

// static boolean nativeRetry(String root);
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeRetry(JNIEnv * env, jclass clazz, jstring root)
{
  return GetStorage().RetryDownloadNode(jni::ToNativeString(env, root));
}

// static boolean nativeUpdate(String root);
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeUpdate(JNIEnv * env, jclass clazz, jstring root)
{
  // FIXME (trashkalmar): Uncomment after method is implemented.
  //return GetStorage().UpdateNode(jni::ToNativeString(env, root));
  return true;
}

// static boolean nativeCancel(String root);
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeCancel(JNIEnv * env, jclass clazz, jstring root)
{
  return GetStorage().CancelDownloadNode(jni::ToNativeString(env, root));
}

// static boolean nativeDelete(String root);
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeDelete(JNIEnv * env, jclass clazz, jstring root)
{
  return GetStorage().DeleteNode(jni::ToNativeString(env, root));
}

static void StatusChangedCallback(shared_ptr<jobject> const & listenerRef, TCountryId const & countryId)
{
  JNIEnv * env = jni::GetEnv();

  // TODO: The core will do this itself
  NodeAttrs attrs;
  GetStorage().GetNodeAttrs(countryId, attrs);

  jmethodID const methodID = jni::GetMethodID(env, *listenerRef.get(), "onStatusChanged", "(Ljava/lang/String;I)V");
  env->CallVoidMethod(*listenerRef.get(), methodID, jni::ToJavaString(env, countryId), attrs.m_status);
}

static void ProgressChangedCallback(shared_ptr<jobject> const & listenerRef, TCountryId const & countryId, TLocalAndRemoteSize const & sizes)
{
  JNIEnv * env = jni::GetEnv();

  jmethodID const methodID = jni::GetMethodID(env, *listenerRef.get(), "onProgress", "(Ljava/lang/String;JJ)V");
  env->CallVoidMethod(*listenerRef.get(), methodID, jni::ToJavaString(env, countryId), sizes.first, sizes.second);
}

// static int nativeSubscribe(StorageCallback listener);
JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeSubscribe(JNIEnv * env, jclass clazz, jobject listener)
{
  PrepareClassRefs(env);
  return GetStorage().Subscribe(bind(&StatusChangedCallback, jni::make_global_ref(listener), _1),
                                bind(&ProgressChangedCallback, jni::make_global_ref(listener), _1, _2));
}

// static void nativeUnsubscribe(int slot);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeUnsubscribe(JNIEnv * env, jclass clazz, jint slot)
{
  GetStorage().Unsubscribe(slot);
}

} // extern "C"
