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

#include "platform/http_client.hpp"

#include "base/logging.hpp"
#include "base/assert.hpp"

#include "std/string.hpp"

namespace
{
template <typename T, typename D>
unique_ptr<T, D> MakeScopedPointer(T * ptr, D deleter)
{
  return unique_ptr<T, D>(ptr, deleter);
}

// Scoped environment which can attach to any thread and automatically detach
class ScopedEnv final
{
public:
  ScopedEnv(JavaVM * vm)
  {
    JNIEnv * env;
    auto result = vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    if (result == JNI_EDETACHED)
    {
      result = vm->AttachCurrentThread(&env, nullptr);
      m_needToDetach = (result == JNI_OK);
    }

    if (result == JNI_OK)
    {
      m_env = env;
      m_vm = vm;
    }
  }

  ~ScopedEnv()
  {
    if (m_vm != nullptr && m_needToDetach)
      m_vm->DetachCurrentThread();
  }

  JNIEnv * operator->() { return m_env; }
  operator bool() const { return m_env != nullptr; }
  JNIEnv * get() { return m_env; }

private:
  bool m_needToDetach = false;
  JNIEnv * m_env = nullptr;
  JavaVM * m_vm = nullptr;
};
}  // namespace

#define CLEAR_AND_RETURN_FALSE_ON_EXCEPTION \
  if (env->ExceptionCheck()) {              \
    env->ExceptionDescribe();               \
    env->ExceptionClear();                  \
    return false;                           \
  }

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

  auto const deleter = [&env](jobject o) { env->DeleteLocalRef(o); };

  // Create and fill request params.
  auto const jniUrl = MakeScopedPointer(jni::ToJavaString(env.get(), m_urlRequested), deleter);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  static jmethodID const httpParamsConstructor =
      env->GetMethodID(g_httpParamsClazz, "<init>", "(Ljava/lang/String;)V");

  auto const httpParamsObject =
      MakeScopedPointer(env->NewObject(g_httpParamsClazz, httpParamsConstructor, jniUrl.get()), deleter);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  // Cache it on the first call.
  static jfieldID const dataField = env->GetFieldID(g_httpParamsClazz, "data", "[B");
  if (!m_bodyData.empty())
  {
    auto const jniPostData = MakeScopedPointer(env->NewByteArray(m_bodyData.size()), deleter);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetByteArrayRegion(jniPostData.get(), 0, m_bodyData.size(),
                            reinterpret_cast<const jbyte *>(m_bodyData.data()));
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), dataField, jniPostData.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  ASSERT(!m_httpMethod.empty(), ("Http method type can not be empty."));
  static jfieldID const httpMethodField =
      env->GetFieldID(g_httpParamsClazz, "httpMethod", "Ljava/lang/String;");
  {
    const auto jniHttpMethod = MakeScopedPointer(jni::ToJavaString(env.get(), m_httpMethod), deleter);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), httpMethodField, jniHttpMethod.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  static jfieldID const contentTypeField =
      env->GetFieldID(g_httpParamsClazz, "contentType", "Ljava/lang/String;");
  if (!m_contentType.empty())
  {
    auto const jniContentType = MakeScopedPointer(jni::ToJavaString(env.get(), m_contentType), deleter);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), contentTypeField, jniContentType.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  static jfieldID const contentEncodingField =
      env->GetFieldID(g_httpParamsClazz, "contentEncoding", "Ljava/lang/String;");
  if (!m_contentEncoding.empty())
  {
    auto const jniContentEncoding = MakeScopedPointer(jni::ToJavaString(env.get(), m_contentEncoding), deleter);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), contentEncodingField, jniContentEncoding.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  if (!m_userAgent.empty())
  {
    static jfieldID const userAgentField =
        env->GetFieldID(g_httpParamsClazz, "userAgent", "Ljava/lang/String;");

    auto const jniUserAgent = MakeScopedPointer(jni::ToJavaString(env.get(), m_userAgent), deleter);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), userAgentField, jniUserAgent.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  if (!m_bodyFile.empty())
  {
    static jfieldID const inputFilePathField =
        env->GetFieldID(g_httpParamsClazz, "inputFilePath", "Ljava/lang/String;");

    auto const jniInputFilePath = MakeScopedPointer(jni::ToJavaString(env.get(), m_bodyFile), deleter);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), inputFilePathField, jniInputFilePath.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  if (!m_receivedFile.empty())
  {
    static jfieldID const outputFilePathField =
        env->GetFieldID(g_httpParamsClazz, "outputFilePath", "Ljava/lang/String;");

    auto const jniOutputFilePath = MakeScopedPointer(jni::ToJavaString(env.get(), m_receivedFile), deleter);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), outputFilePathField, jniOutputFilePath.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  if (!m_basicAuthUser.empty())
  {
    static jfieldID const basicAuthUserField =
        env->GetFieldID(g_httpParamsClazz, "basicAuthUser", "Ljava/lang/String;");

    auto const jniBasicAuthUser = MakeScopedPointer(jni::ToJavaString(env.get(), m_basicAuthUser), deleter);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), basicAuthUserField, jniBasicAuthUser.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  if (!m_basicAuthPassword.empty())
  {
    static jfieldID const basicAuthPasswordField =
        env->GetFieldID(g_httpParamsClazz, "basicAuthPassword", "Ljava/lang/String;");

    auto const jniBasicAuthPassword =
       MakeScopedPointer(jni::ToJavaString(env.get(), m_basicAuthPassword), deleter);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), basicAuthPasswordField, jniBasicAuthPassword.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  static jfieldID const cookiesField =
      env->GetFieldID(g_httpParamsClazz, "cookies", "Ljava/lang/String;");
  if (!m_cookies.empty())
  {
    const auto jniCookies = MakeScopedPointer(jni::ToJavaString(env.get(), m_cookies), deleter);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), cookiesField, jniCookies.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
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

  static jfieldID const receivedUrlField =
      env->GetFieldID(g_httpParamsClazz, "receivedUrl", "Ljava/lang/String;");
  auto const jniReceivedUrl =
      MakeScopedPointer(static_cast<jstring>(env->GetObjectField(response, receivedUrlField)), deleter);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniReceivedUrl)
    m_urlReceived = jni::ToNativeString(env.get(), jniReceivedUrl.get());

  // contentTypeField is already cached above.
  auto const jniContentType =
      MakeScopedPointer(static_cast<jstring>(env->GetObjectField(response, contentTypeField)), deleter);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniContentType)
    m_contentTypeReceived = jni::ToNativeString(env.get(), jniContentType.get());

  // contentEncodingField is already cached above.
  auto const jniContentEncoding =
      MakeScopedPointer(static_cast<jstring>(env->GetObjectField(response, contentEncodingField)), deleter);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniContentEncoding)
    m_contentEncodingReceived = jni::ToNativeString(env.get(), jniContentEncoding.get());

  // Note: cookies field is used not only to send Cookie header, but also to receive back
  // Server-Cookie header. CookiesField is already cached above.
  auto const jniServerCookies =
      MakeScopedPointer(static_cast<jstring>(env->GetObjectField(response, cookiesField)), deleter);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniServerCookies)
    m_serverCookies =
        normalize_server_cookies(std::move(jni::ToNativeString(env.get(), jniServerCookies.get())));

  // dataField is already cached above.
  auto const jniData =
      MakeScopedPointer(static_cast<jbyteArray>(env->GetObjectField(response, dataField)), deleter);
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
