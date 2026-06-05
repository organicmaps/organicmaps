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
JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeSetColor(JNIEnv *, jclass, jlong bmk, jint color)
{
  auto const * mark = getBookmark(bmk);
  auto const bmColor = kml::kOrderedPredefinedColors[color];

  if (mark->GetColor() == bmColor)
    return;  // New color is the same as existing color. Nothing to update.

  // initialize new bookmark
  kml::BookmarkData bmData(mark->GetData());
  bmData.m_color.m_predefinedColor = bmColor;

  g_framework->ReplaceBookmark(static_cast<kml::MarkId>(bmk), bmData);
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeUpdateParams(JNIEnv * env, jclass, jlong bmk,
                                                                                   jstring name, jint color,
                                                                                   jstring descr)
{
  auto const * mark = getBookmark(bmk);

  // initialize new bookmark
  auto const bmName = jni::ToNativeString(env, name);
  auto const bmDescr = jni::ToNativeString(env, descr);
  auto const bmColor = kml::kOrderedPredefinedColors[color];

  if (mark->GetPreferredName() == bmName && mark->GetDescription() == bmDescr && mark->GetColor() == bmColor)
    return;  // New bookmark parameters match existing params. Nothing to update.

  kml::BookmarkData bmData(mark->GetData());

  if (mark->GetPreferredName() != bmName)
    kml::SetDefaultStr(bmData.m_customName, bmName);
  kml::SetDefaultStr(bmData.m_description, bmDescr);
  bmData.m_color.m_predefinedColor = bmColor;

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
