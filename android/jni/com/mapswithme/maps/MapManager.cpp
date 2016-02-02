#include "Framework.hpp"

#include "../core/jni_helper.hpp"

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
  OTHER,
};

enum ItemStatus : uint32_t
{
  UPDATABLE,
  DOWNLOADABLE,
  ENQUEUED,
  DONE,
  PROGRESS,
  FAILED,
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

string GetLocalizedName(string const & id)
{
  // TODO
  return id;
}

} // namespace data

extern "C"
{

using namespace storage;
using namespace data;

// static native boolean nativeIsLegacyMode();
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeIsLegacyMode(JNIEnv * env, jclass clazz)
{
  // TODO (trashkalmar): use appropriate method
  return version::IsSingleMwm(g_framework->Storage().GetCurrentDataVersion());
}

// static @Nullable UpdateInfo nativeGetUpdateInfo();
JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeGetUpdateInfo(JNIEnv * env, jclass clazz)
{
  // FIXME (trashkalmar): Uncomment after Storage::GetUpdateInfo() is implemented
  static Storage::UpdateInfo info = { 0 };
  //if (!GetStorage().GetUpdateInfo(info))
  //  return nullptr;

  static jclass const infoClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/downloader/UpdateInfo");
  ASSERT(infoClass, (jni::DescribeException()));
  static jmethodID const ctor = env->GetMethodID(infoClass, "<init>", "(II)V");
  ASSERT(ctor, (jni::DescribeException()));

  return env->NewObject(infoClass, ctor, info.m_numberOfMwmFilesToUpdate, info.m_totalUpdateSizeInBytes);
}

static void PutItemsToList(JNIEnv * env, jobject const list,  vector<TCountryId> const & children, TCountryId const & parent, function<void (jobject const)> const & callback)
{
  static jmethodID const countryItemCtor = env->GetMethodID(g_countryItemClass, "<init>", "()V");
  static jfieldID const countryItemFieldId = env->GetFieldID(g_countryItemClass, "id", "Ljava/lang/String;");
  static jfieldID const countryItemFieldParentId = env->GetFieldID(g_countryItemClass, "parentId", "Ljava/lang/String;");
  static jfieldID const countryItemFieldName = env->GetFieldID(g_countryItemClass, "name", "Ljava/lang/String;");
  static jfieldID const countryItemFieldParentName = env->GetFieldID(g_countryItemClass, "parentName", "Ljava/lang/String;");

  jstring parentId = jni::ToJavaString(env, parent);
  jstring parentName = jni::ToJavaString(env, GetLocalizedName(parent));

  for (TCountryId const & child : children)
  {
    jobject item = env->NewObject(g_countryItemClass, countryItemCtor);

    // ID and parent`s ID
    jstring id = jni::ToJavaString(env, child);
    env->SetObjectField(item, countryItemFieldId, id);
    env->SetObjectField(item, countryItemFieldParentId, parentId);

    // Localized name and parent`s name
    jstring name = jni::ToJavaString(env, GetLocalizedName(child));
    env->SetObjectField(item, countryItemFieldName, name);
    env->SetObjectField(item, countryItemFieldParentName, parentName);

    // Let the caller do special processing
    callback(item);

    // Put to resulting list
    env->CallBooleanMethod(list, g_listAddMethod, item);

    // Drop local refs
    env->DeleteLocalRef(item);
    env->DeleteLocalRef(id);
    env->DeleteLocalRef(name);
  }
}

// static void nativeListItems(@Nullable String parent, List<CountryItem> result);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeListItems(JNIEnv * env, jclass clazz, jstring parent, jobject result)
{
  PrepareClassRefs(env);

  Storage const & storage = GetStorage();
  TCountryId const parentId = (parent ? jni::ToNativeString(env, parent) : storage.GetRootId());

  if (parent)
  {
    vector<TCountryId> children;
    storage.GetChildren(parentId, children);
    PutItemsToList(env, result, children, parentId, [](jobject const item)
    {

    });
  }
  else
  {
    // TODO (trashkalmar): Countries near me

    // Downloaded
    vector<TCountryId> children;
    storage.GetDownloadedChildren(parentId, children);

    PutItemsToList(env, result, children, parentId, [](jobject const item)
    {

    });

    //
  }

  //vector<TCountryId> const children = storage::GetChildren(parentId);
}

// static boolean nativeStartDownload(String countryId);
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeStartDownload(JNIEnv * env, jclass clazz, jstring countryId)
{
  // FIXME (trashkalmar): Uncomment after Storage::DownloadNode() is implemented
  return true;//GetStorage().DownloadNode(jni::ToNativeString(env, countryId));
}

// static boolean nativeCancelDownload(String countryId);
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeCancelDownload(JNIEnv * env, jclass clazz, jstring countryId)
{
  // FIXME (trashkalmar): Uncomment after Storage::DeleteNode() is implemented
  return true;//GetStorage().DeleteNode(jni::ToNativeString(env, countryId));
}

static void StatusChangedCallback(shared_ptr<jobject> const & listenerRef, TCountryId const & countryId)
{
  JNIEnv * env = jni::GetEnv();

  jmethodID const methodID = jni::GetJavaMethodID(env, *listenerRef.get(), "onStatusChanged", "(Ljava/lang/String;)V");
  env->CallVoidMethod(*listenerRef.get(), methodID, jni::ToJavaString(env, countryId));
}

static void ProgressChangedCallback(shared_ptr<jobject> const & listenerRef, TCountryId const & countryId, LocalAndRemoteSizeT const & sizes)
{
  JNIEnv * env = jni::GetEnv();

  jmethodID const methodID = jni::GetJavaMethodID(env, *listenerRef.get(), "onProgress", "(Ljava/lang/String;JJ)V");
  env->CallVoidMethod(*listenerRef.get(), methodID, jni::ToJavaString(env, countryId), sizes.first, sizes.second);
}

// static int nativeSubscribe(StorageCallback listener);
JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeSubscribe(JNIEnv * env, jclass clazz, jobject listener)
{
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