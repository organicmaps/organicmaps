#include <jni.h>

#include "com/mapswithme/core/jni_helper.hpp"

#include "base/logging.hpp"
#include "editor/osm_auth.hpp"

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

jobjectArray ToStringArray(JNIEnv * env, string const & secret, string const & token, string const & url)
{
  jobjectArray resultArray = env->NewObjectArray(3, GetStringClass(env), nullptr);
  env->SetObjectArrayElement(resultArray, 0, ToJavaString(env, secret));
  env->SetObjectArrayElement(resultArray, 1, ToJavaString(env, token));
  env->SetObjectArrayElement(resultArray, 2, ToJavaString(env, url));
  return resultArray;
}
} // namespace

extern "C"
{

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_OsmOAuth_nativeAuthWithPassword(JNIEnv * env, jclass clazz,
                                                                jstring login, jstring password)
{
  OsmOAuth auth = OsmOAuth::ServerAuth();
  auto authResult = auth.AuthorizePassword(ToNativeString(env, login), ToNativeString(env, password));
  return authResult == OsmOAuth::AuthResult::OK ? ToStringArray(env, auth.GetToken())
                                                : nullptr;
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_OsmOAuth_nativeAuthWithWebviewToken(JNIEnv * env, jclass clazz,
                                                                    jstring secret, jstring token, jstring verifier)
{
  OsmOAuth auth = OsmOAuth::ServerAuth();
  TKeySecret outKeySecret;
  TKeySecret inKeySecret(ToNativeString(env, secret), ToNativeString(env, token));
  auto authResult = auth.FinishAuthorization(inKeySecret, ToNativeString(env, verifier), outKeySecret);
  if (authResult != OsmOAuth::AuthResult::OK)
    return nullptr;
  auth.FinishAuthorization(inKeySecret, ToNativeString(env, token), outKeySecret);
  return ToStringArray(env, outKeySecret);
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_OsmOAuth_nativeGetFacebookAuthUrl(JNIEnv * env, jclass clazz)
{
  OsmOAuth::TUrlKeySecret keySecret = OsmOAuth::ServerAuth().GetFacebookOAuthURL();
  return ToStringArray(env, keySecret.first, keySecret.second.first, keySecret.second.second);
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_OsmOAuth_nativeGetGoogleAuthUrl(JNIEnv * env, jclass clazz)
{
  OsmOAuth::TUrlKeySecret keySecret = OsmOAuth::ServerAuth().GetGoogleOAuthURL();
  return ToStringArray(env, keySecret.first, keySecret.second.first, keySecret.second.second);
}
} // extern "C"
