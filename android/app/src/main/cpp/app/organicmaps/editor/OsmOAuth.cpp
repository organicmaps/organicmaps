#include <jni.h>

#include "app/organicmaps/core/jni_helper.hpp"
#include "app/organicmaps/Framework.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "editor/osm_auth.hpp"
#include "editor/server_api.hpp"

namespace
{
using namespace osm;
using namespace jni;

jobjectArray ToStringArray(JNIEnv * env, KeySecret const & secret)
{
  jobjectArray resultArray = env->NewObjectArray(2, GetStringClass(env), nullptr);
  env->SetObjectArrayElement(resultArray, 0, ToJavaString(env, secret.first));
  env->SetObjectArrayElement(resultArray, 1, ToJavaString(env, secret.second));
  return resultArray;
}

// @returns [url, key, secret]
jobjectArray ToStringArray(JNIEnv * env, OsmOAuth::UrlRequestToken const & uks)
{
  jobjectArray resultArray = env->NewObjectArray(3, GetStringClass(env), nullptr);
  env->SetObjectArrayElement(resultArray, 0, ToJavaString(env, uks.first));
  env->SetObjectArrayElement(resultArray, 1, ToJavaString(env, uks.second.first));
  env->SetObjectArrayElement(resultArray, 2, ToJavaString(env, uks.second.second));
  return resultArray;
}

bool LoadOsmUserPreferences(KeySecret const & keySecret, UserPreferences & outPrefs)
{
  try
  {
    ServerApi06 const api(OsmOAuth::ServerAuth(keySecret));
    outPrefs = api.GetUserPreferences();
    return true;
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("Can't load user preferences from server: ", ex.what()));
  }
  return false;
}
}  // namespace

extern "C"
{

JNIEXPORT jobjectArray JNICALL
Java_app_organicmaps_editor_OsmOAuth_nativeAuthWithPassword(JNIEnv * env, jclass clazz,
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
Java_app_organicmaps_editor_OsmOAuth_nativeAuthWithWebviewToken(JNIEnv * env, jclass clazz,
                                                                    jstring key, jstring secret, jstring verifier)
{
  try
  {
    RequestToken const rt = { ToNativeString(env, key), ToNativeString(env, secret) };
    OsmOAuth auth = OsmOAuth::ServerAuth();
    KeySecret const ks = auth.FinishAuthorization(rt, ToNativeString(env, verifier));
    return ToStringArray(env, ks);
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("nativeAuthWithWebviewToken error ", ex.what()));
    return nullptr;
  }
}

JNIEXPORT jobjectArray JNICALL
Java_app_organicmaps_editor_OsmOAuth_nativeGetGoogleAuthUrl(JNIEnv * env, jclass clazz)
{
  try
  {
    OsmOAuth::UrlRequestToken const uks = OsmOAuth::ServerAuth().GetGoogleOAuthURL();
    return ToStringArray(env, uks);
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("nativeGetGoogleAuthUrl error ", ex.what()));
    return nullptr;
  }
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_editor_OsmOAuth_nativeGetOsmUsername(JNIEnv * env, jclass, jstring token, jstring secret)
{
  const KeySecret keySecret(jni::ToNativeString(env, token), jni::ToNativeString(env, secret));
  UserPreferences prefs;
  if (LoadOsmUserPreferences(keySecret, prefs))
    return jni::ToJavaString(env, prefs.m_displayName);
  return nullptr;
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_editor_OsmOAuth_nativeGetOsmChangesetsCount(JNIEnv * env, jclass, jstring token, jstring secret)
{
  const KeySecret keySecret(jni::ToNativeString(env, token), jni::ToNativeString(env, secret));
  UserPreferences prefs;
  if (LoadOsmUserPreferences(keySecret, prefs))
    return prefs.m_changesets;
  return -1;
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_editor_OsmOAuth_nativeGetHistoryUrl(JNIEnv * env, jclass, jstring user)
{
  return jni::ToJavaString(env, OsmOAuth::ServerAuth().GetHistoryURL(jni::ToNativeString(env, user)));
}
} // extern "C"
