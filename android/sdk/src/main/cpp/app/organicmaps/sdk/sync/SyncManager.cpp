#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "map/bookmark_helpers.hpp"
#include "platform/platform.hpp"

#include "base/assert.hpp"

using namespace jni;

namespace sync_manager
{
jobject g_syncManagerInstance;
jmethodID g_onFileChangedMethod;

void InitMethodIds(JNIEnv * env, jobject syncManagerInstance)
{
  g_syncManagerInstance = env->NewGlobalRef(syncManagerInstance);
  g_onFileChangedMethod = jni::GetMethodID(env, syncManagerInstance, "onFileChanged", "(Ljava/lang/String;)V");
}

void OnFileChanged(JNIEnv * env, std::string filePath)
{
  ASSERT(g_syncManagerInstance, ());
  GetPlatform().RunTask(Platform::Thread::Gui, [env, filePath = std::move(filePath)]()
  {
    jni::TScopedLocalRef jFilePath(env, jni::ToJavaString(env, filePath));
    env->CallVoidMethod(g_syncManagerInstance, g_onFileChangedMethod, jFilePath.get());
    jni::HandleJavaException(env);
  });
}

}  // namespace sync_manager

extern "C"
{
JNIEXPORT void JNICALL
Java_app_organicmaps_sync_SyncManager_nativeInit(JNIEnv * env, jobject thiz)
{
  sync_manager::InitMethodIds(env, thiz);
  frm()->GetBookmarkManager().SetFileChangedCallback(
      std::bind(&sync_manager::OnFileChanged, env, std::placeholders::_1));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sync_SyncManager_nativeAddSuffixToCategory(JNIEnv * env, jclass, jstring filePath)
{
  std::string const path = jni::ToNativeString(env, filePath);
  frm()->GetBookmarkManager().AddSuffixToCategoryName(path);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sync_SyncManager_nativeReloadBookmark(JNIEnv * env, jclass, jstring filePath)
{
  frm()->GetBookmarkManager().ReloadBookmark(ToNativeString(env, filePath));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sync_SyncManager_nativeDeleteBmCategory(JNIEnv * env, jclass, jstring filePath)
{
  auto & bm = frm()->GetBookmarkManager();
  auto const groupId = bm.GetCategoryByFileName(ToNativeString(env, filePath));
  if (groupId != kml::kInvalidMarkGroupId)
    bm.GetEditSession().DeleteBmCategory(groupId, true);
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_sync_SyncManager_nativeGetBookmarksDir(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, GetBookmarksDirectory());
}

JNIEXPORT jobjectArray JNICALL
Java_app_organicmaps_sync_SyncManager_nativeGetLoadedCategoryPaths(JNIEnv * env, jclass)
{
  std::vector<std::string> filePaths;
  auto const & bm = frm()->GetBookmarkManager();
  return jni::ToJavaStringArray(env, bm.GetLoadedCategoryPaths());
}
}  // extern "C"
