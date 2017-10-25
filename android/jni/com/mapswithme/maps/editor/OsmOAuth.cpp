#include <jni.h>

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "editor/osm_auth.hpp"
#include "editor/server_api.hpp"
#include "editor/user_stats.hpp"

namespace
{
using namespace osm;
using namespace jni;

jobjectArray ToStringArray(JNIEnv * env, TKeySecret const & secret)
{
  jobjectArray resultArray = env->NewObjectArray(2, GetStringClass(env), nullptr);
  env->SetObjectArrayElement(resultArray, 0, ToJavaString(env, secret.first));
  env->SetObjectArrayElement(resultArray, 1, ToJavaString(env, secret.second));
  return resultArray;
}

// @returns [url, key, secret]
jobjectArray ToStringArray(JNIEnv * env, OsmOAuth::TUrlRequestToken const & uks)
{
  jobjectArray resultArray = env->NewObjectArray(3, GetStringClass(env), nullptr);
  env->SetObjectArrayElement(resultArray, 0, ToJavaString(env, uks.first));
  env->SetObjectArrayElement(resultArray, 1, ToJavaString(env, uks.second.first));
  env->SetObjectArrayElement(resultArray, 2, ToJavaString(env, uks.second.second));
  return resultArray;
}
}  // namespace

extern "C"
{

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_OsmOAuth_nativeAuthWithPassword(JNIEnv * env, jclass clazz,
                                                                jstring login, jstring password)
{
  OsmOAuth auth = OsmOAuth::ServerAuth();
  try
  {
    if (auth.AuthorizePassword(ToNativeString(env, login), ToNativeString(env, password)))
      return ToStringArray(env, auth.GetKeySecret());
    LOG(LWARNING, ("nativeAuthWithPassword: invalid login or password."));
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("nativeAuthWithPassword error ", ex.what()));
  }
  return nullptr;
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_OsmOAuth_nativeAuthWithWebviewToken(JNIEnv * env, jclass clazz,
                                                                    jstring key, jstring secret, jstring verifier)
{
  try
  {
    TRequestToken const rt = { ToNativeString(env, key), ToNativeString(env, secret) };
    OsmOAuth auth = OsmOAuth::ServerAuth();
    TKeySecret const ks = auth.FinishAuthorization(rt, ToNativeString(env, verifier));
    return ToStringArray(env, ks);
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("nativeAuthWithWebviewToken error ", ex.what()));
    return nullptr;
  }
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_OsmOAuth_nativeGetFacebookAuthUrl(JNIEnv * env, jclass clazz)
{
  try
  {
    OsmOAuth::TUrlRequestToken const uks = OsmOAuth::ServerAuth().GetFacebookOAuthURL();
    return ToStringArray(env, uks);
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("nativeGetFacebookAuthUrl error ", ex.what()));
    return nullptr;
  }
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_OsmOAuth_nativeGetGoogleAuthUrl(JNIEnv * env, jclass clazz)
{
  try
  {
    OsmOAuth::TUrlRequestToken const uks = OsmOAuth::ServerAuth().GetGoogleOAuthURL();
    return ToStringArray(env, uks);
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("nativeGetGoogleAuthUrl error ", ex.what()));
    return nullptr;
  }
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_OsmOAuth_nativeGetOsmUsername(JNIEnv * env, jclass clazz, jstring token, jstring secret)
{
  try
  {
    TKeySecret keySecret(jni::ToNativeString(env, token), jni::ToNativeString(env, secret));
    ServerApi06 const api(OsmOAuth::ServerAuth(keySecret));
    return jni::ToJavaString(env, api.GetUserPreferences().m_displayName);
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("Can't load user preferences from server: ", ex.what()));
    return nullptr;
  }
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_OsmOAuth_nativeUpdateOsmUserStats(JNIEnv * env, jclass clazz, jstring jUsername, jboolean forceUpdate)
{
  static jclass const statsClazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/editor/data/UserStats");
  static jmethodID const statsCtor = jni::GetConstructorID(env, statsClazz, "(IILjava/lang/String;J)V");
  static jclass const osmAuthClazz = static_cast<jclass>(env->NewGlobalRef(clazz));
  // static void onUserStatsUpdated(UserStats stats)
  static jmethodID const listenerId = jni::GetStaticMethodID(env, osmAuthClazz, "onUserStatsUpdated", "(Lcom/mapswithme/maps/editor/data/UserStats;)V");

  std::string const username = jni::ToNativeString(env, jUsername);
  auto const policy = forceUpdate ? editor::UserStatsLoader::UpdatePolicy::Force
                                  : editor::UserStatsLoader::UpdatePolicy::Lazy;
  g_framework->NativeFramework()->UpdateUserStats(username, policy, [username]()
  {
    editor::UserStats const & userStats = g_framework->NativeFramework()->GetUserStats(username);
    if (!userStats.IsValid())
      return;
    int32_t count, rank;
    std::string levelUp;
    userStats.GetChangesCount(count);
    userStats.GetRank(rank);
    userStats.GetLevelUpRequiredFeat(levelUp);
    JNIEnv * env = jni::GetEnv();
    env->CallStaticVoidMethod(osmAuthClazz, listenerId,
                              env->NewObject(statsClazz, statsCtor, count, rank, jni::ToJavaString(env, levelUp),
                                             my::TimeTToSecondsSinceEpoch(userStats.GetLastUpdate())));
  });
}
} // extern "C"
