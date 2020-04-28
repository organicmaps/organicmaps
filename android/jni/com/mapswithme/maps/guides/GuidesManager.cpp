#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"
#include "com/mapswithme/platform/Platform.hpp"
#include <jni.h>
using namespace std::placeholders;

extern "C"
{
  static void GuidesStateChanged(GuidesManager::GuidesState state,
                                 std::shared_ptr<jobject> const & listener)
  {
    LOG(LINFO, (static_cast<int>(state)));
    JNIEnv * env = jni::GetEnv();
    env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener, "onStateChanged", "(I)V"),
                        static_cast<jint>(state));
  }

  JNIEXPORT void JNICALL Java_com_mapswithme_maps_maplayer_guides_GuidesManager_nativeAddListener(
      JNIEnv * env, jclass clazz, jobject listener)
  {
    CHECK(g_framework, ("Framework isn't created yet!"));
    g_framework->SetGuidesListener(
        std::bind(&GuidesStateChanged, std::placeholders::_1, jni::make_global_ref(listener)));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_maplayer_guides_GuidesManager_nativeRemoveListener(JNIEnv * env,
                                                                              jclass clazz)
  {
    CHECK(g_framework, ("Framework isn't created yet!"));
    g_framework->SetGuidesListener(nullptr);
  }
}
