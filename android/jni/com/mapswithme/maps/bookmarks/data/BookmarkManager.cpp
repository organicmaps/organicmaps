#include "../../Framework.hpp"

#include "../../../core/jni_helper.hpp"
#include "../../../../../../../base/logging.hpp"

namespace {
  ::Framework * frm() { return g_framework->NativeFramework(); }
}

extern "C"
{

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_putBookmark(
      JNIEnv * env, jobject thiz, jint x, jint y, jstring bookmarkName, jstring categoryName)
  {
    Bookmark bm(frm()->PtoG(m2::PointD(x, y)), jni::ToNativeString(env, bookmarkName), "placemark-red");
    LOG(LDEBUG, ("lan lon ",bm.GetOrg().x,bm.GetOrg().y));
    frm()->AddBookmark(jni::ToNativeString(env, categoryName), bm);
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_getCategoriesCount(
       JNIEnv * env, jobject thiz)
  {
    return frm()->GetBmCategoriesCount();
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nDeleteCategory(
       JNIEnv * env, jobject thiz, jint index)
  {
    return frm()->DeleteBmCategory(index);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nDeleteBookmark(JNIEnv * env, jobject thiz, jint cat, jint bmk)
  {
    frm()->GetBmCategory(cat)->DeleteBookmark(bmk);
  }
}
