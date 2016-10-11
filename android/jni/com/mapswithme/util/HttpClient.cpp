/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/
#include <jni.h>

#include "../core/jni_helper.hpp"
#include "../core/ScopedEnv.hpp"
#include "../core/ScopedLocalRef.hpp"

#include "platform/http_client.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"

#include "std/string.hpp"
#include "std/unordered_map.hpp"

DECLARE_EXCEPTION(JniException, RootException);

#define CLEAR_AND_RETURN_FALSE_ON_EXCEPTION \
  if (env->ExceptionCheck())                \
  {                                         \
    env->ExceptionDescribe();               \
    env->ExceptionClear();                  \
    return false;                           \
  }

namespace
{
void RethrowOnJniException(ScopedEnv & env)
{
  if (!env->ExceptionCheck())
    return;

  env->ExceptionDescribe();
  env->ExceptionClear();
  MYTHROW(JniException, ());
}

jfieldID GetHttpParamsFieldId(ScopedEnv & env, const char * name)
{
  return env->GetFieldID(g_httpParamsClazz, name, "Ljava/lang/String;");
}

// Set string value to HttpClient.Params object, throws JniException and
void SetString(ScopedEnv & env, jobject params, jfieldID const fieldId, string const & value)
{
  if (value.empty())
    return;

  jni::ScopedLocalRef<jstring> const wrappedValue(env.get(), jni::ToJavaString(env.get(), value));
  RethrowOnJniException(env);

  env->SetObjectField(params, fieldId, wrappedValue.get());
  RethrowOnJniException(env);
}

// Get string value from HttpClient.Params object, throws JniException.
void GetString(ScopedEnv & env, jobject const params, jfieldID const fieldId, string & result)
{
  jni::ScopedLocalRef<jstring> const wrappedValue(
      env.get(), static_cast<jstring>(env->GetObjectField(params, fieldId)));
  RethrowOnJniException(env);
  if (wrappedValue)
    result = jni::ToNativeString(env.get(), wrappedValue.get());
}

class Ids
{
public:
  explicit Ids(ScopedEnv & env)
  {
    m_fieldIds =
    {{"httpMethod", GetHttpParamsFieldId(env, "httpMethod")},
    {"contentType", GetHttpParamsFieldId(env, "contentType")},
    {"contentEncoding", GetHttpParamsFieldId(env, "contentEncoding")},
    {"userAgent", GetHttpParamsFieldId(env, "userAgent")},
    {"inputFilePath", GetHttpParamsFieldId(env, "inputFilePath")},
    {"outputFilePath", GetHttpParamsFieldId(env, "outputFilePath")},
    {"basicAuthUser", GetHttpParamsFieldId(env, "basicAuthUser")},
    {"basicAuthPassword", GetHttpParamsFieldId(env, "basicAuthPassword")},
    {"cookies", GetHttpParamsFieldId(env, "cookies")},
    {"receivedUrl", GetHttpParamsFieldId(env, "receivedUrl")}};
  }

  jfieldID GetId(string const & fieldName) const
  {
    auto const it = m_fieldIds.find(fieldName);
    CHECK(it != m_fieldIds.end(), ("Incorrect field name:", fieldName));
    return it->second;
  }

private:
  unordered_map<string, jfieldID> m_fieldIds;
};
}  // namespace

//***********************************************************************
// Exported functions implementation
//***********************************************************************
namespace platform
{
bool HttpClient::RunHttpRequest()
{
  ScopedEnv env(jni::GetJVM());

  if (!env)
    return false;

  static Ids ids(env);

  // Create and fill request params.
  jni::ScopedLocalRef<jstring> const jniUrl(env.get(),
                                            jni::ToJavaString(env.get(), m_urlRequested));
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  static jmethodID const httpParamsConstructor =
      env->GetMethodID(g_httpParamsClazz, "<init>", "(Ljava/lang/String;)V");

  jni::ScopedLocalRef<jobject> const httpParamsObject(
      env.get(), env->NewObject(g_httpParamsClazz, httpParamsConstructor, jniUrl.get()));
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  // Cache it on the first call.
  static jfieldID const dataField = env->GetFieldID(g_httpParamsClazz, "data", "[B");
  if (!m_bodyData.empty())
  {
    jni::ScopedLocalRef<jbyteArray> const jniPostData(env.get(),
                                                      env->NewByteArray(m_bodyData.size()));
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetByteArrayRegion(jniPostData.get(), 0, m_bodyData.size(),
                            reinterpret_cast<const jbyte *>(m_bodyData.data()));
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), dataField, jniPostData.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  ASSERT(!m_httpMethod.empty(), ("Http method type can not be empty."));

  try
  {
    SetString(env, httpParamsObject.get(), ids.GetId("httpMethod"), m_httpMethod);
    SetString(env, httpParamsObject.get(), ids.GetId("contentType"), m_contentType);
    SetString(env, httpParamsObject.get(), ids.GetId("contentEncoding"), m_contentEncoding);
    SetString(env, httpParamsObject.get(), ids.GetId("userAgent"), m_userAgent);
    SetString(env, httpParamsObject.get(), ids.GetId("inputFilePath"), m_inputFile);
    SetString(env, httpParamsObject.get(), ids.GetId("outputFilePath"), m_outputFile);
    SetString(env, httpParamsObject.get(), ids.GetId("basicAuthUser"), m_basicAuthUser);
    SetString(env, httpParamsObject.get(), ids.GetId("basicAuthPassword"), m_basicAuthPassword);
    SetString(env, httpParamsObject.get(), ids.GetId("cookies"), m_cookies);
  }
  catch (JniException const & ex)
  {
    return false;
  }

  static jfieldID const debugModeField = env->GetFieldID(g_httpParamsClazz, "debugMode", "Z");
  env->SetBooleanField(httpParamsObject.get(), debugModeField, m_debugMode);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  static jfieldID const followRedirectsField =
      env->GetFieldID(g_httpParamsClazz, "followRedirects", "Z");
  env->SetBooleanField(httpParamsObject.get(), followRedirectsField, m_handleRedirects);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  static jmethodID const httpClientClassRun =
    env->GetStaticMethodID(g_httpClientClazz, "run",
        "(Lcom/mapswithme/util/HttpClient$Params;)Lcom/mapswithme/util/HttpClient$Params;");

  // Current Java implementation simply reuses input params instance, so we don't need to
  // call DeleteLocalRef(response).
  jobject const response =
      env->CallStaticObjectMethod(g_httpClientClazz, httpClientClassRun, httpParamsObject.get());
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  static jfieldID const httpResponseCodeField =
      env->GetFieldID(g_httpParamsClazz, "httpResponseCode", "I");
  m_errorCode = env->GetIntField(response, httpResponseCodeField);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  try
  {
    GetString(env, response, ids.GetId("receivedUrl"), m_urlReceived);
    GetString(env, response, ids.GetId("contentType"), m_contentTypeReceived);
    GetString(env, response, ids.GetId("contentEncoding"), m_contentEncodingReceived);
  }
  catch (JniException const & ex)
  {
    return false;
  }

  // Note: cookies field is used not only to send Cookie header, but also to receive back
  // Server-Cookie header. CookiesField is already cached above.
  jni::ScopedLocalRef<jstring> const jniServerCookies(
      env.get(), static_cast<jstring>(env->GetObjectField(response, ids.GetId("cookies"))));
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniServerCookies)
  {
    m_serverCookies =
        NormalizeServerCookies(jni::ToNativeString(env.get(), jniServerCookies.get()));
  }

  // dataField is already cached above.
  jni::ScopedLocalRef<jbyteArray> const jniData(
      env.get(), static_cast<jbyteArray>(env->GetObjectField(response, dataField)));
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniData)
  {
    jbyte * buffer = env->GetByteArrayElements(jniData.get(), nullptr);
    if (buffer)
    {
      m_serverResponse.assign(reinterpret_cast<const char *>(buffer), env->GetArrayLength(jniData.get()));
      env->ReleaseByteArrayElements(jniData.get(), buffer, JNI_ABORT);
    }
  }
  return true;
}
}  // namespace platform
