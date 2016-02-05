#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"
#include "com/mapswithme/maps/UserMarkHelper.hpp"

#include "coding/zip_creator.hpp"


namespace
{
::Framework * frm() { return g_framework->NativeFramework(); }
}

extern "C"
{
using namespace jni;

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeShowBookmarkOnMap(
    JNIEnv * env, jobject thiz, jint c, jint b)
{
  BookmarkAndCategory bnc = BookmarkAndCategory(c,b);
  g_framework->PostDrapeTask([bnc]()
  {
    frm()->ShowBookmark(bnc);
    frm()->SaveState();
  });
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
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteBookmark(
    JNIEnv * env, jobject thiz, jint cat, jint bmk)
{
  BookmarkCategory * pCat = frm()->GetBmCategory(cat);
  if (pCat)
  {
    BookmarkCategory::Guard guard(*pCat);
    guard.m_controller.DeleteUserMark(bmk);
    pCat->SaveToKMLFile();
  }
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteTrack(
    JNIEnv * env, jobject thiz, jint cat, jint trk)
{
  BookmarkCategory * pCat = frm()->GetBmCategory(cat);
  if (pCat)
  {
    pCat->DeleteTrack(trk);
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
    string const name = pCat->GetName();
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
  BookmarkAndCategory const bmkAndCat = g_framework->AddBookmark(f->LastEditedBMCategory(), glbPoint, bmkData);
  BookmarkCategory::Guard guard(*f->GetBookmarkManager().GetBmCategory(bmkAndCat.first));
  UserMark * newBookmark = guard.m_controller.GetUserMarkForEdit(bmkAndCat.second);
  // TODO @deathbaba or @yunikkk - remove that hack after BookmarkManager will correctly set features for bookmarks.
  newBookmark->SetFeature(f->GetFeatureAtPoint(glbPoint));
  g_framework->SetActiveUserMark(newBookmark);
  return usermark_helper::CreateMapObject(newBookmark);
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_getLastEditedCategory(
      JNIEnv * env, jobject thiz)
{
  return frm()->LastEditedBMCategory();
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGenerateUniqueBookmarkName(JNIEnv * env, jclass thiz, jstring jBaseName)
{
  string baseName = ToNativeString(env, jBaseName);
  string bookmarkFileName = BookmarkCategory::GenerateUniqueFileName(GetPlatform().SettingsDir(), baseName);
  return ToJavaString(env, bookmarkFileName);
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeLoadKmzFile(JNIEnv * env, jobject thiz, jstring path)
{
  return frm()->AddBookmarksFile(ToNativeString(env, path));
}
}
