#include "map/bookmark.hpp"
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/bookmarks/data/Metadata.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

jobject CreateBookmark(JNIEnv * env, place_page::Info const & info, jni::TScopedLocalObjectArrayRef const & jrawTypes,
                       jni::TScopedLocalRef const & routingPointInfo)
{
  // clang-format off
  static jmethodID const ctorId = jni::GetConstructorID(env, g_bookmarkClazz,
    "("
    "J"                                               // categoryId
    "J"                                               // bookmarkId
    "Ljava/lang/String;"                              // title
    "Ljava/lang/String;"                              // secondaryTitle
    "Ljava/lang/String;"                              // subtitle
    "Ljava/lang/String;"                              // address
    "Lapp/organicmaps/sdk/routing/RoutePointInfo;"    // routePointInfo
    "I"                                               // openingMode
    "Ljava/lang/String;"                              // wikiArticle
    "Ljava/lang/String;"                              // osmDescription
    "[Ljava/lang/String;"                             // rawTypes
    ")V"
  );
  // clang-format on

  // clang-format off
  jobject mapObject = env->NewObject(g_bookmarkClazz, ctorId,
    static_cast<jlong>(info.GetBookmarkCategoryId()),
    static_cast<jlong>(info.GetBookmarkId()),
    jni::ToJavaStringWithSupplementalCharsFix(env, info.GetTitle()),
    jni::ToJavaStringWithSupplementalCharsFix(env, info.GetSecondaryTitle()),
    jni::ToJavaStringWithSupplementalCharsFix(env, info.GetSubtitle()),
    jni::ToJavaStringWithSupplementalCharsFix(env, info.GetSecondarySubtitle()),
    routingPointInfo.get(),
    info.GetOpeningMode(),
    jni::ToJavaString(env, info.GetWikiDescription()),
    jni::ToJavaString(env, info.GetOSMDescription()),
    jrawTypes.get()
  );
  // clang-format on
  if (info.HasMetadata())
    InjectMetadata(env, g_mapObjectClazz, mapObject, info);
  return mapObject;
}

Bookmark const * getBookmark(jlong bokmarkId)
{
  Bookmark const * pBmk = frm()->GetBookmarkManager().GetBookmark(static_cast<kml::MarkId>(bokmarkId));
  ASSERT(pBmk, ("Bookmark not found, id", bokmarkId));
  return pBmk;
}

extern "C"
{
JNIEXPORT jint Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeGetColor(JNIEnv *, jclass, jlong bmk)
{
  auto const * mark = getBookmark(bmk);
  return static_cast<jint>(
      kml::kColorIndexMap[base::E2I(mark != nullptr ? mark->GetColor() : frm()->LastEditedBMColor())]);
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeSetColor(JNIEnv *, jclass, jlong bmk, jint color)
{
  auto const * mark = getBookmark(bmk);

  // initialize new bookmark
  kml::BookmarkData bmData(mark->GetData());
  bmData.m_color.m_predefinedColor = kml::kOrderedPredefinedColors[color];

  g_framework->ReplaceBookmark(static_cast<kml::MarkId>(bmk), bmData);
}

JNIEXPORT jint Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeGetIcon(JNIEnv *, jclass, jlong bmk)
{
  auto const * mark = getBookmark(bmk);
  return static_cast<jint>(mark != nullptr ? mark->GetData().m_icon : kml::BookmarkIcon::None);
}

JNIEXPORT jobject Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeGetXY(JNIEnv * env, jclass, jlong bmk)
{
  return jni::GetNewParcelablePointD(env, getBookmark(bmk)->GetPivot());
}

JNIEXPORT jstring Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeGetFeatureType(JNIEnv * env, jclass, jlong bmk)
{
  return jni::ToJavaString(env, kml::GetLocalizedFeatureType(getBookmark(bmk)->GetData().m_featureTypes));
}

JNIEXPORT jstring Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeGetName(JNIEnv * env, jclass, jlong bmk)
{
  return jni::ToJavaString(env, getBookmark(bmk)->GetPreferredName());
}

JNIEXPORT jstring Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeGetDescription(JNIEnv * env, jclass, jlong bmk)
{
  return jni::ToJavaString(env, getBookmark(bmk)->GetDescription());
}

JNIEXPORT jdouble Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeGetScale(JNIEnv *, jclass, jlong bmk)
{
  return getBookmark(bmk)->GetScale();
}

JNIEXPORT jstring Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeGetAddress(JNIEnv * env, jclass, jlong bmkId)
{
  auto const address = frm()->GetAddressAtPoint(getBookmark(bmkId)->GetPivot()).FormatAddress();
  return jni::ToJavaString(env, address);
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeUpdateParams(JNIEnv * env, jclass, jlong bmk,
                                                                                   jstring name, jint color,
                                                                                   jstring descr)
{
  auto const * mark = getBookmark(bmk);

  // initialize new bookmark
  kml::BookmarkData bmData(mark->GetData());
  auto const bmName = jni::ToNativeString(env, name);
  if (mark->GetPreferredName() != bmName)
    kml::SetDefaultStr(bmData.m_customName, bmName);
  if (descr)
    kml::SetDefaultStr(bmData.m_description, jni::ToNativeString(env, descr));
  bmData.m_color.m_predefinedColor = kml::kOrderedPredefinedColors[color];

  g_framework->ReplaceBookmark(static_cast<kml::MarkId>(bmk), bmData);
}

JNIEXPORT jstring Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeEncode2Ge0Url(JNIEnv * env, jclass, jlong bmk,
                                                                                       jboolean addName)
{
  return jni::ToJavaString(env, frm()->CodeGe0url(getBookmark(bmk), addName));
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeChangeCategory(JNIEnv *, jclass, jlong oldCatId,
                                                                                     jlong newCatId, jlong bookmarkId)
{
  g_framework->MoveBookmark(static_cast<kml::MarkId>(bookmarkId), static_cast<kml::MarkGroupId>(oldCatId),
                            static_cast<kml::MarkGroupId>(newCatId));
}
}  // extern "C"
