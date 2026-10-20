#include "map/bookmark.hpp"
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/bookmarks/data/Metadata.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

jobject CreateBookmark(JNIEnv * env, place_page::Info const & info)
{
  // clang-format off
  static jmethodID ctorId = jni::GetConstructorID(env, g_bookmarkClazz,
    "("
    "J"                                               // categoryId
    "J"                                               // bookmarkId
    "Ljava/lang/String;"                              // title
    "Ljava/lang/String;"                              // secondaryTitle
    "Ljava/lang/String;"                              // subtitle
    "Ljava/lang/String;"                              // address
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
    jni::ToJavaStringWithSupplementalCharsFix(env, info.GetSecondarySubtitle())
  );
  // clang-format on
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

  // initialize new bookmark
  kml::BookmarkData bmData(mark->GetData());
  bmData.m_color = kml::MakeCustomBookmarkColorData(dp::Color::FromARGB(color));

  g_framework->ReplaceBookmark(static_cast<kml::MarkId>(bmk), bmData);
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
  bmData.m_color = kml::MakeCustomBookmarkColorData(dp::Color::FromARGB(color));

  g_framework->ReplaceBookmark(static_cast<kml::MarkId>(bmk), bmData);
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_Bookmark_nativeChangeCategory(JNIEnv *, jclass, jlong oldCatId,
                                                                                     jlong newCatId, jlong bookmarkId)
{
  g_framework->MoveBookmark(static_cast<kml::MarkId>(bookmarkId), static_cast<kml::MarkGroupId>(oldCatId),
                            static_cast<kml::MarkGroupId>(newCatId));
}
}  // extern "C"
