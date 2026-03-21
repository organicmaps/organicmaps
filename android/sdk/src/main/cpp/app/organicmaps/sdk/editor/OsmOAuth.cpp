#include <jni.h>

#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "editor/osm_auth.hpp"
#include "editor/server_api.hpp"

namespace
{
bool LoadOsmUserPreferences(std::string const & oauthToken, osm::UserPreferences & outPrefs)
{
  try
  {
    osm::ServerApi06 const api(osm::OsmOAuth::ServerAuth(oauthToken));
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
JNIEXPORT jstring Java_app_organicmaps_sdk_editor_OsmOAuth_nativeGetOAuth2Url(JNIEnv * env, jclass)
{
  auto const auth = osm::OsmOAuth::ServerAuth();
  return jni::ToJavaString(env, auth.BuildOAuth2Url());
}

// Attempts to authenticate with login and password, returns true on success or false on failure
JNIEXPORT jboolean Java_app_organicmaps_sdk_editor_OsmOAuth_nativeAuthWithPassword(JNIEnv * env, jclass clazz,
                                                                                   jstring login, jstring password,
                                                                                   jobjectArray result)
{
  osm::OsmOAuth auth = osm::OsmOAuth::ServerAuth();

  try
  {
    if (auth.AuthorizePassword(jni::ToNativeString(env, login), jni::ToNativeString(env, password)))
    {
      env->SetObjectArrayElement(result, 0, jni::ToJavaString(env, auth.GetAuthToken()));
      if (env->ExceptionCheck())
        return JNI_FALSE;
      return JNI_TRUE;
    }
    LOG(LWARNING, ("nativeAuthWithPassword: invalid login or password."));
    env->SetObjectArrayElement(result, 0, jni::ToJavaString(env, "Invalid login or password"));
    if (env->ExceptionCheck())
      return JNI_FALSE;
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("nativeAuthWithPassword error ", ex.what()));
    env->SetObjectArrayElement(result, 0, jni::ToJavaString(env, ex.what()));
    if (env->ExceptionCheck())
      return JNI_FALSE;
  }
  return JNI_FALSE;
}

JNIEXPORT jstring Java_app_organicmaps_sdk_editor_OsmOAuth_nativeAuthWithOAuth2Code(JNIEnv * env, jclass,
                                                                                    jstring oauth2code)
{
  osm::OsmOAuth auth = osm::OsmOAuth::ServerAuth();
  try
  {
    auto token = auth.FinishAuthorization(jni::ToNativeString(env, oauth2code));
    if (!token.empty())
    {
      auth.SetAuthToken(token);
      return jni::ToJavaString(env, token);
    }
    LOG(LWARNING, ("nativeAuthWithOAuth2Code: invalid OAuth2 code", oauth2code));
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("nativeAuthWithOAuth2Code error ", ex.what()));
  }
  return nullptr;
}

JNIEXPORT jstring Java_app_organicmaps_sdk_editor_OsmOAuth_nativeGetOsmUsername(JNIEnv * env, jclass,
                                                                                jstring oauthToken)
{
  osm::UserPreferences prefs;
  if (LoadOsmUserPreferences(jni::ToNativeString(env, oauthToken), prefs))
    return jni::ToJavaString(env, prefs.m_displayName);
  return nullptr;
}

JNIEXPORT jint Java_app_organicmaps_sdk_editor_OsmOAuth_nativeGetOsmChangesetsCount(JNIEnv * env, jclass,
                                                                                    jstring oauthToken)
{
  osm::UserPreferences prefs;
  if (LoadOsmUserPreferences(jni::ToNativeString(env, oauthToken), prefs))
    return prefs.m_changesets;
  return -1;
}

JNIEXPORT jstring Java_app_organicmaps_sdk_editor_OsmOAuth_nativeGetOsmProfilePictureUrl(JNIEnv * env, jclass,
                                                                                         jstring oauthToken)
{
  osm::UserPreferences prefs;
  if (LoadOsmUserPreferences(jni::ToNativeString(env, oauthToken), prefs))
    return jni::ToJavaString(env, prefs.m_imageUrl);
  return nullptr;
}

JNIEXPORT jstring Java_app_organicmaps_sdk_editor_OsmOAuth_nativeGetHistoryUrl(JNIEnv * env, jclass, jstring user)
{
  return jni::ToJavaString(env, osm::OsmOAuth::ServerAuth().GetHistoryURL(jni::ToNativeString(env, user)));
}

JNIEXPORT jstring Java_app_organicmaps_sdk_editor_OsmOAuth_nativeGetNotesUrl(JNIEnv * env, jclass, jstring user)
{
  return jni::ToJavaString(env, osm::OsmOAuth::ServerAuth().GetNotesURL(jni::ToNativeString(env, user)));
}
}  // extern "C"
