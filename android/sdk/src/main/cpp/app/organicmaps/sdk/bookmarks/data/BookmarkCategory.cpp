#include "BookmarkCategory.hpp"

#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

namespace
{
inline jclass getBookmarkCategoryClass(JNIEnv * env)
{
  static jclass g_bookmarkCategoryClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/BookmarkCategory");
  return g_bookmarkCategoryClass;
}
}  // namespace

jobject ToJavaBookmarkCategory(JNIEnv * env, kml::MarkGroupId id)
{
  // clang-format off
  static jmethodID g_bookmarkCategoryConstructor = jni::GetConstructorID(env, getBookmarkCategoryClass(env),
    "("
    "J"                   // id
    "Ljava/lang/String;"  // name
    "Ljava/lang/String;"  // annotation
    "Ljava/lang/String;"  // desc
    "I"                   // tracksCount
    "I"                   // bookmarksCount
    "Z"                   // isVisible
    ")V"
  );
  // clang-format on

  auto const & manager = frm()->GetBookmarkManager();
  auto const & data = manager.GetCategoryData(id);

  auto const tracksCount = manager.GetTrackIds(data.m_id).size();
  auto const bookmarksCount = manager.GetUserMarkIds(data.m_id).size();
  auto const isVisible = manager.IsVisible(data.m_id);
  auto const preferBookmarkStr = GetPreferredBookmarkStr(data.m_name);
  auto const annotation = GetPreferredBookmarkStr(data.m_annotation);
  auto const description = GetPreferredBookmarkStr(data.m_description);

  jni::TScopedLocalRef preferBookmarkStrRef(env, jni::ToJavaString(env, preferBookmarkStr));
  jni::TScopedLocalRef annotationRef(env, jni::ToJavaString(env, annotation));
  jni::TScopedLocalRef descriptionRef(env, jni::ToJavaString(env, description));

  // clang-format off
  return env->NewObject(getBookmarkCategoryClass(env), g_bookmarkCategoryConstructor,
    static_cast<jlong>(data.m_id),
    preferBookmarkStrRef.get(),
    annotationRef.get(),
    descriptionRef.get(),
    static_cast<jint>(tracksCount),
    static_cast<jint>(bookmarksCount),
    static_cast<jboolean>(isVisible)
  );
  // clang-format on
}

jobjectArray ToJavaBookmarkCategories(JNIEnv * env, kml::GroupIdCollection const & ids)
{
  return jni::ToJavaArray(env, getBookmarkCategoryClass(env), ids, std::bind(&ToJavaBookmarkCategory, _1, _2));
}

extern "C"
{
JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeSetName(JNIEnv * env, jclass, jlong catId,
                                                                                      jstring name)
{
  frm()->GetBookmarkManager().GetEditSession().SetCategoryName(static_cast<kml::MarkGroupId>(catId),
                                                               jni::ToNativeString(env, name));
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeSetDescription(JNIEnv * env, jclass,
                                                                                             jlong catId, jstring desc)
{
  frm()->GetBookmarkManager().GetEditSession().SetCategoryDescription(static_cast<kml::MarkGroupId>(catId),
                                                                      jni::ToNativeString(env, desc));
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeIsVisible(JNIEnv *, jclass,
                                                                                            jlong catId)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().IsVisible(static_cast<kml::MarkGroupId>(catId)));
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeSetVisibility(JNIEnv *, jclass,
                                                                                            jlong catId,
                                                                                            jboolean isVisible)
{
  frm()->GetBookmarkManager().GetEditSession().SetIsVisible(static_cast<kml::MarkGroupId>(catId), isVisible);
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeSetTags(JNIEnv * env, jclass, jlong catId,
                                                                                      jobjectArray tagsIds)
{
  auto const size = env->GetArrayLength(tagsIds);
  std::vector<std::string> categoryTags;
  categoryTags.reserve(static_cast<size_t>(size));
  for (auto i = 0; i < size; i++)
  {
    jni::TScopedLocalRef const item(env, env->GetObjectArrayElement(tagsIds, i));
    categoryTags.push_back(jni::ToNativeString(env, static_cast<jstring>(item.get())));
  }

  frm()->GetBookmarkManager().GetEditSession().SetCategoryTags(static_cast<kml::MarkGroupId>(catId), categoryTags);
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeSetAccessRules(JNIEnv *, jclass,
                                                                                             jlong catId,
                                                                                             jint accessRules)
{
  frm()->GetBookmarkManager().GetEditSession().SetCategoryAccessRules(static_cast<kml::MarkGroupId>(catId),
                                                                      static_cast<kml::AccessRules>(accessRules));
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeSetCustomProperty(JNIEnv * env, jclass,
                                                                                                jlong catId,
                                                                                                jstring key,
                                                                                                jstring value)
{
  frm()->GetBookmarkManager().GetEditSession().SetCategoryCustomProperty(
      static_cast<kml::MarkGroupId>(catId), jni::ToNativeString(env, key), jni::ToNativeString(env, value));
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeIsEmpty(JNIEnv *, jclass, jlong catId)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().IsCategoryEmpty(static_cast<kml::MarkGroupId>(catId)));
}

JNIEXPORT jlong Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeGetBookmarkIdByPosition(
    JNIEnv *, jclass, jlong catId, jint positionInCategory)
{
  auto const & ids = frm()->GetBookmarkManager().GetUserMarkIds(static_cast<kml::MarkGroupId>(catId));
  if (positionInCategory >= static_cast<jlong>(ids.size()))
    return static_cast<jlong>(kml::kInvalidMarkId);
  auto it = ids.begin();
  std::advance(it, positionInCategory);
  return static_cast<jlong>(*it);
}

JNIEXPORT jlong Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeGetTrackIdByPosition(
    JNIEnv *, jclass, jlong catId, jint positionInCategory)
{
  auto const & ids = frm()->GetBookmarkManager().GetTrackIds(static_cast<kml::MarkGroupId>(catId));
  if (positionInCategory >= static_cast<jlong>(ids.size()))
    return static_cast<jlong>(kml::kInvalidTrackId);
  auto it = ids.begin();
  std::advance(it, positionInCategory);
  return static_cast<jlong>(*it);
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeHasLastSortingType(JNIEnv *, jclass, jlong catId)
{
  auto const & bm = frm()->GetBookmarkManager();
  BookmarkManager::SortingType type;
  return static_cast<jboolean>(bm.GetLastSortingType(static_cast<kml::MarkGroupId>(catId), type));
}

JNIEXPORT jint Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeGetLastSortingType(JNIEnv *, jclass,
                                                                                                 jlong catId)
{
  auto const & bm = frm()->GetBookmarkManager();
  BookmarkManager::SortingType type;
  auto const hasType = bm.GetLastSortingType(static_cast<kml::MarkGroupId>(catId), type);
  ASSERT(hasType, ());
  UNUSED_VALUE(hasType);
  return static_cast<jint>(type);
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeSetLastSortingType(JNIEnv *, jclass,
                                                                                                 jlong catId, jint type)
{
  auto & bm = frm()->GetBookmarkManager();
  bm.SetLastSortingType(static_cast<kml::MarkGroupId>(catId), static_cast<BookmarkManager::SortingType>(type));
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeResetLastSortingType(JNIEnv *, jclass,
                                                                                                   jlong catId)
{
  auto & bm = frm()->GetBookmarkManager();
  bm.ResetLastSortingType(static_cast<kml::MarkGroupId>(catId));
}

JNIEXPORT jintArray Java_app_organicmaps_sdk_bookmarks_data_BookmarkCategory_nativeGetAvailableSortingTypes(
    JNIEnv * env, jclass, jlong catId, jboolean hasMyPosition)
{
  auto const & bm = frm()->GetBookmarkManager();
  auto const types =
      bm.GetAvailableSortingTypes(static_cast<kml::MarkGroupId>(catId), static_cast<bool>(hasMyPosition));
  int const size = static_cast<int>(types.size());
  jintArray jTypes = env->NewIntArray(size);
  jint * arr = env->GetIntArrayElements(jTypes, 0);
  for (int i = 0; i < size; ++i)
    arr[i] = static_cast<int>(types[i]);
  env->ReleaseIntArrayElements(jTypes, arr, 0);

  return jTypes;
}
}  // extern "C"
