#include "Framework.hpp"

#include "../core/jni_helper.hpp"

#include "coding/internal/file_data.hpp"
#include "platform/mwm_version.hpp"
#include "storage/storage.hpp"

#include "std/bind.hpp"
#include "std/shared_ptr.hpp"

namespace
{

using namespace storage;

enum ItemCategory : uint32_t
{
  NEAR_ME,
  DOWNLOADED,
  ALL,
};

jmethodID g_listAddMethod;
jclass g_countryItemClass;
jobject g_countryChangedListener;

Storage & GetStorage()
{
  return g_framework->Storage();
}

void PrepareClassRefs(JNIEnv * env)
{
  if (g_listAddMethod)
    return;

  jclass listClass = env->FindClass("java/util/List");
  g_listAddMethod = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");
  g_countryItemClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/downloader/CountryItem");
}

}  // namespace

extern "C"
{
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
  Storage::UpdateInfo info = { 0 };
  if (!GetStorage().GetUpdateInfo(GetStorage().GetRootId(), info))
    return nullptr;

  static jclass const infoClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/downloader/UpdateInfo");
  ASSERT(infoClass, (jni::DescribeException()));
  static jmethodID const ctor = jni::GetConstructorID(env, infoClass, "(II)V");
  ASSERT(ctor, (jni::DescribeException()));

  return env->NewObject(infoClass, ctor, info.m_numberOfMwmFilesToUpdate, info.m_totalUpdateSizeInBytes);
}

static void UpdateItem(JNIEnv * env, jobject item, NodeAttrs const & attrs)
{
  static jfieldID const countryItemFieldName = env->GetFieldID(g_countryItemClass, "name", "Ljava/lang/String;");
  static jfieldID const countryItemFieldParentId = env->GetFieldID(g_countryItemClass, "parentId", "Ljava/lang/String;");
  static jfieldID const countryItemFieldParentName = env->GetFieldID(g_countryItemClass, "parentName", "Ljava/lang/String;");
  static jfieldID const countryItemFieldSize = env->GetFieldID(g_countryItemClass, "size", "J");
  static jfieldID const countryItemFieldTotalSize = env->GetFieldID(g_countryItemClass, "totalSize", "J");
  static jfieldID const countryItemFieldChildCount = env->GetFieldID(g_countryItemClass, "childCount", "I");
  static jfieldID const countryItemFieldTotalChildCount = env->GetFieldID(g_countryItemClass, "totalChildCount", "I");
  static jfieldID const countryItemFieldStatus = env->GetFieldID(g_countryItemClass, "status", "I");
  static jfieldID const countryItemFieldErrorCode = env->GetFieldID(g_countryItemClass, "errorCode", "I");
  static jfieldID const countryItemFieldPresent = env->GetFieldID(g_countryItemClass, "present", "Z");
  static jfieldID const countryItemFieldProgress = env->GetFieldID(g_countryItemClass, "progress", "I");

  // Localized name
  jni::TScopedLocalRef const name(env, jni::ToJavaString(env, attrs.m_nodeLocalName));
  env->SetObjectField(item, countryItemFieldName, name.get());

  // Info about parent[s]. Do not specify if there are multiple parents or none.
  if (attrs.m_parentInfo.size() == 1)
  {
    CountryIdAndName const & info = attrs.m_parentInfo[0];

    jni::TScopedLocalRef const parentId(env, jni::ToJavaString(env, info.m_id));
    env->SetObjectField(item, countryItemFieldParentId, parentId.get());

    jni::TScopedLocalRef const parentName(env, jni::ToJavaString(env, info.m_localName));
    env->SetObjectField(item, countryItemFieldParentName, parentName.get());
  }
  else
  {
    env->SetObjectField(item, countryItemFieldParentId, nullptr);
    env->SetObjectField(item, countryItemFieldParentName, nullptr);
  }

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

  // Progress
  env->SetIntField(item, countryItemFieldProgress,static_cast<jint>(attrs.m_downloadingProgress.first));
}

static void PutItemsToList(JNIEnv * env, jobject const list,  vector<TCountryId> const & children, TCountryId const & parent, int category)
{
  static jmethodID const countryItemCtor = jni::GetConstructorID(env, g_countryItemClass, "(Ljava/lang/String;)V");
  static jfieldID const countryItemFieldCategory = env->GetFieldID(g_countryItemClass, "category", "I");

  NodeAttrs attrs;
  for (TCountryId const & child : children)
  {
    GetStorage().GetNodeAttrs(child, attrs);

    jni::TScopedLocalRef const id(env, jni::ToJavaString(env, child));
    jni::TScopedLocalRef const item(env, env->NewObject(g_countryItemClass, countryItemCtor, id.get()));
    env->SetIntField(item.get(), countryItemFieldCategory, category);

    UpdateItem(env, item.get(), attrs);

    // Put to resulting list
    env->CallBooleanMethod(list, g_listAddMethod, item.get());
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

  if (parent)
  {
    vector<TCountryId> children;
    storage.GetChildren(parentId, children);
    PutItemsToList(env, result, children, parentId, ItemCategory::ALL);
  }
  else
  {
    // TODO (trashkalmar): Countries near me

    // Downloaded
    vector<TCountryId> children;
    storage.GetDownloadedChildren(parentId, children);
    PutItemsToList(env, result, children, parentId, ItemCategory::DOWNLOADED);

    // All
    storage.GetChildren(parentId, children);
    PutItemsToList(env, result, children, parentId, ItemCategory::ALL);
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

// static void nativeDelete(String root);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeDelete(JNIEnv * env, jclass clazz, jstring root)
{
  GetStorage().DeleteNode(jni::ToNativeString(env, root));
}

static void StatusChangedCallback(shared_ptr<jobject> const & listenerRef, TCountryId const & countryId)
{
  JNIEnv * env = jni::GetEnv();

  // TODO: The core will do this itself
  NodeAttrs attrs;
  GetStorage().GetNodeAttrs(countryId, attrs);

  jmethodID const methodID = jni::GetMethodID(env, *listenerRef, "onStatusChanged", "(Ljava/lang/String;I)V");
  env->CallVoidMethod(*listenerRef, methodID, jni::ToJavaString(env, countryId), attrs.m_status);
}

static void ProgressChangedCallback(shared_ptr<jobject> const & listenerRef, TCountryId const & countryId, TLocalAndRemoteSize const & sizes)
{
  JNIEnv * env = jni::GetEnv();

  jmethodID const methodID = jni::GetMethodID(env, *listenerRef, "onProgress", "(Ljava/lang/String;JJ)V");
  env->CallVoidMethod(*listenerRef, methodID, jni::ToJavaString(env, countryId), sizes.first, sizes.second);
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

// static void nativeSubscribeOnCountryChanged(CurrentCountryChangedListener listener);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeSubscribeOnCountryChanged(JNIEnv * env, jclass clazz, jobject listener)
{
  ASSERT(!g_countryChangedListener, ());
  g_countryChangedListener = env->NewGlobalRef(listener);

  auto const callback = [](TCountryId const & countryId)
  {
    JNIEnv * env = jni::GetEnv();
    jmethodID methodID = jni::GetMethodID(env, g_countryChangedListener, "onCurrentCountryChanged", "(Ljava/lang/String;)V");
    env->CallVoidMethod(g_countryChangedListener, methodID, jni::TScopedLocalRef(env, jni::ToJavaString(env, countryId)).get());
  };

  TCountryId const & prev = g_framework->NativeFramework()->GetLastReportedCountry();
  g_framework->NativeFramework()->SetCurrentCountryChangedListener(callback);

  // Report previous value
  callback(prev);
}

// static void nativeUnsubscribeOnCountryChanged();
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeUnsubscribeOnCountryChanged(JNIEnv * env, jclass clazz)
{
  g_framework->NativeFramework()->SetCurrentCountryChangedListener(nullptr);

  ASSERT(g_countryChangedListener, ());
  env->DeleteGlobalRef(g_countryChangedListener);
  g_countryChangedListener = nullptr;
}

} // extern "C"
