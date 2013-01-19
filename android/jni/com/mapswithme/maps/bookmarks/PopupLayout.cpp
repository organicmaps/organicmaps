#include "../Framework.hpp"

#include "../../core/jni_helper.hpp"

namespace
{
  ::Framework * frm() { return g_framework->NativeFramework(); }
}

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_PopupLayout_nDrawBookmark(JNIEnv * env, jobject thiz, jdouble px, jdouble py)
  {
    frm()->DrawPlacemark(frm()->PtoG(m2::PointD(px, py)));
    frm()->Invalidate();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_PopupLayout_nRemoveBookmark(JNIEnv * env, jobject thiz)
  {
    frm()->DisablePlacemark();
    frm()->Invalidate();
  }
}
