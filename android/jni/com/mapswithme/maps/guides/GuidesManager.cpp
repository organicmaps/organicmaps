#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"
#include "com/mapswithme/maps/guides/Guides.hpp"
#include "com/mapswithme/platform/Platform.hpp"

#include <jni.h>

namespace
{
jclass g_guidesManagerClass = nullptr;
jmethodID g_guidesManagerFromMethod = nullptr;
jmethodID g_onGalleryChangedMethod = nullptr;
std::shared_ptr<jobject>  g_guidesManager = nullptr;

void PrepareClassRefs(JNIEnv *env)
{
  if (g_guidesManagerClass != nullptr)
    return;

  g_guidesManagerClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/maplayer/guides/GuidesManager");
  g_guidesManagerFromMethod = jni::GetStaticMethodID(
    env, g_guidesManagerClass, "from",
    "(Landroid/content/Context;)Lcom/mapswithme/maps/maplayer/guides/GuidesManager;");
  auto context = android::Platform::Instance().GetContext();
  g_guidesManager = jni::make_global_ref(env->CallStaticObjectMethod(g_guidesManagerClass,
                                                                     g_guidesManagerFromMethod,
                                                                     context));
  g_onGalleryChangedMethod = jni::GetMethodID(env, *g_guidesManager, "onGalleryChanged", "(Z)V");
  jni::HandleJavaException(env);
}

static void GuidesStateChanged(GuidesManager::GuidesState state,
                               std::shared_ptr<jobject> const & listener)
{
  JNIEnv * env = jni::GetEnv();
  env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener, "onStateChanged", "(I)V"),
                      static_cast<jint>(state));
  jni::HandleJavaException(env);
}

static void GalleryChanged(bool reload)
{
  JNIEnv * env = jni::GetEnv();
  PrepareClassRefs(env);
  env->CallVoidMethod(*g_guidesManager, g_onGalleryChangedMethod, static_cast<jboolean>(reload));
  jni::HandleJavaException(env);
}
} // namespace

extern "C"
{
using namespace std::placeholders;

JNIEXPORT void JNICALL Java_com_mapswithme_maps_maplayer_guides_GuidesManager_nativeSetGuidesStateChangedListener(
  JNIEnv * env, jclass clazz, jobject listener)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  g_framework->SetGuidesListener(
    std::bind(&GuidesStateChanged, std::placeholders::_1, jni::make_global_ref(listener)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_maplayer_guides_GuidesManager_nativeRemoveGuidesStateChangedListener(
  JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  g_framework->SetGuidesListener(nullptr);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_maplayer_guides_GuidesManager_nativeSetActiveGuide(
  JNIEnv * env, jclass clazz, jstring guideId)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetGuidesManager();
  manager.SetActiveGuide(jni::ToNativeString(env, guideId));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_maplayer_guides_GuidesManager_nativeGetActiveGuide(
  JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetGuidesManager();
  return jni::ToJavaString(env, manager.GetActiveGuide());
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_maplayer_guides_GuidesManager_nativeGetGallery(
  JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetGuidesManager();
  auto const gallery = manager.GetGallery();
  return guides::CreateGallery(env, gallery);
}

JNIEXPORT void JNICALL Java_com_mapswithme_maps_maplayer_guides_GuidesManager_nativeSetGalleryChangedListener(
  JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetGuidesManager();
  manager.SetGalleryListener(std::bind(&GalleryChanged, std::placeholders::_1));
}

JNIEXPORT void JNICALL Java_com_mapswithme_maps_maplayer_guides_GuidesManager_nativeRemoveGalleryChangedListener(
  JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetGuidesManager();
  manager.SetGalleryListener(nullptr);
}
}
