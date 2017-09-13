#include "BookmarkManager.hpp"

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"
#include "com/mapswithme/maps/UserMarkHelper.hpp"

#include "coding/zip_creator.hpp"
#include "map/place_page_info.hpp"

namespace
{
::Framework * frm() { return g_framework->NativeFramework(); }
}  // namespace

namespace bookmarks_helper
{
void RemoveBookmark(int cat, int bmk)
{
  BookmarkCategory * pCat = frm()->GetBmCategory(cat);
  if (pCat)
  {
    pCat->DeleteUserMark(bmk);
    pCat->NotifyChanges();
    pCat->SaveToKMLFile();
  }
}
}  // namespace bookmarks_helper

extern "C"
{
using namespace jni;

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeShowBookmarkOnMap(
    JNIEnv * env, jobject thiz, jint c, jint b)
{
  BookmarkAndCategory bnc = BookmarkAndCategory(b, c);
  frm()->ShowBookmark(bnc);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeLoadBookmarks(JNIEnv * env, jobject thiz)
{
  frm()->LoadBookmarks();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetCategoriesCount(JNIEnv * env, jobject thiz)
{
  return frm()->GetBmCategoriesCount();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeCreateCategory(
     JNIEnv * env, jobject thiz, jstring name)
{
  return frm()->AddCategory(ToNativeString(env, name));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteCategory(
     JNIEnv * env, jobject thiz, jint index)
{
  return frm()->DeleteBmCategory(index);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteBookmark(JNIEnv *, jobject, jint cat, jint bmk)
{
  bookmarks_helper::RemoveBookmark(cat, bmk);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteTrack(
    JNIEnv * env, jobject thiz, jint cat, jint trk)
{
  BookmarkCategory * pCat = frm()->GetBmCategory(cat);
  if (pCat)
  {
    pCat->DeleteTrack(trk);
    pCat->NotifyChanges();
    pCat->SaveToKMLFile();
  }
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSaveToKmzFile(
    JNIEnv * env, jobject thiz, jint catID, jstring tmpPath)
{
  BookmarkCategory * pCat = frm()->GetBmCategory(catID);
  if (pCat)
  {
    std::string const name = pCat->GetName();
    if (CreateZipFromPathDeflatedAndDefaultCompression(pCat->GetFileName(), ToNativeString(env, tmpPath) + name + ".kmz"))
      return ToJavaString(env, name);
  }

  return nullptr;
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeAddBookmarkToLastEditedCategory(
    JNIEnv * env, jobject thiz, jstring name, double lat, double lon)
{
  m2::PointD const glbPoint(MercatorBounds::FromLatLon(lat, lon));
  ::Framework * f = frm();
  BookmarkData bmkData(ToNativeString(env, name), f->LastEditedBMType());
  size_t const lastEditedCategory = f->LastEditedBMCategory();
  size_t const createdBookmarkIndex = f->AddBookmark(lastEditedCategory, glbPoint, bmkData);

  place_page::Info & info = g_framework->GetPlacePageInfo();
  BookmarkCategory * category = f->GetBmCategory(lastEditedCategory);
  f->FillBookmarkInfo(*static_cast<Bookmark const *>(category->GetUserMark(createdBookmarkIndex)),
                      {static_cast<size_t>(createdBookmarkIndex),
                       static_cast<size_t>(lastEditedCategory)}, info);

  return usermark_helper::CreateMapObject(env, info);
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_getLastEditedCategory(
      JNIEnv * env, jobject thiz)
{
  return frm()->LastEditedBMCategory();
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGenerateUniqueFileName(JNIEnv * env, jclass thiz, jstring jBaseName)
{
  std::string baseName = ToNativeString(env, jBaseName);
  std::string bookmarkFileName = BookmarkCategory::GenerateUniqueFileName(GetPlatform().SettingsDir(), baseName);
  return ToJavaString(env, bookmarkFileName);
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeLoadKmzFile(JNIEnv * env, jobject thiz, jstring path)
{
  return frm()->AddBookmarksFile(ToNativeString(env, path));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeFormatNewBookmarkName(JNIEnv * env, jclass)
{
  return ToJavaString(env, g_framework->GetPlacePageInfo().FormatNewBookmarkName());
}
}  // extern "C"
