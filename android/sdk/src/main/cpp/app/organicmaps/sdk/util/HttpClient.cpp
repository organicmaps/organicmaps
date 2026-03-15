/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Alexander Borsuk <me@alex.bio> from Minsk, Belarus

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

#include "app/organicmaps/sdk/core/ScopedEnv.hpp"
#include "app/organicmaps/sdk/core/ScopedLocalRef.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "platform/http_client.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>

DECLARE_EXCEPTION(JniException, RootException);

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

jfieldID GetHttpParamsFieldId(ScopedEnv & env, char const * name, char const * signature = "Ljava/lang/String;")
{
  return env->GetFieldID(g_httpParamsClazz, name, signature);
}

void SetString(ScopedEnv & env, jobject params, jfieldID const fieldId, std::string const & value)
{
  if (value.empty())
    return;

  jni::ScopedLocalRef<jstring> const wrappedValue(env.get(), jni::ToJavaString(env.get(), value));
  RethrowOnJniException(env);

  env->SetObjectField(params, fieldId, wrappedValue.get());
  RethrowOnJniException(env);
}

void SetBoolean(ScopedEnv & env, jobject params, jfieldID const fieldId, bool const value)
{
  env->SetBooleanField(params, fieldId, value);
  RethrowOnJniException(env);
}

void SetInt(ScopedEnv & env, jobject params, jfieldID const fieldId, int const value)
{
  env->SetIntField(params, fieldId, value);
  RethrowOnJniException(env);
}

void GetString(ScopedEnv & env, jobject const params, jfieldID const fieldId, std::string & result)
{
  jni::ScopedLocalRef<jstring> const wrappedValue(env.get(),
                                                  static_cast<jstring>(env->GetObjectField(params, fieldId)));
  RethrowOnJniException(env);
  if (wrappedValue)
    result = jni::ToNativeString(env.get(), wrappedValue.get());
}

void GetInt(ScopedEnv & env, jobject const params, jfieldID const fieldId, int & result)
{
  result = env->GetIntField(params, fieldId);
  RethrowOnJniException(env);
}

void SetHeaders(ScopedEnv & env, jobject const params, platform::HttpClient::Headers const & headers)
{
  if (headers.empty())
    return;

  static jmethodID const setHeaders =
      env->GetMethodID(g_httpParamsClazz, "setHeaders", "([Lapp/organicmaps/sdk/util/KeyValue;)V");

  RethrowOnJniException(env);

  jni::TScopedLocalObjectArrayRef jHeaders(env.get(), jni::ToKeyValueArray(env.get(), headers));
  env->CallVoidMethod(params, setHeaders, jHeaders.get());
  RethrowOnJniException(env);
}

void LoadHeaders(ScopedEnv & env, jobject const params, platform::HttpClient::Headers & headers)
{
  static jmethodID const getHeaders = env->GetMethodID(g_httpParamsClazz, "getHeaders", "()[Ljava/lang/Object;");

  jni::ScopedLocalRef<jobjectArray> const headersArray(
      env.get(), static_cast<jobjectArray>(env->CallObjectMethod(params, getHeaders)));

  RethrowOnJniException(env);

  headers.clear();
  jni::ToNativekeyValueContainer(env.get(), headersArray, std::inserter(headers, headers.end()));

  RethrowOnJniException(env);
}

class Ids
{
public:
  explicit Ids(ScopedEnv & env)
  {
    m_fieldIds = {{"httpMethod", GetHttpParamsFieldId(env, "httpMethod")},
                  {"inputFilePath", GetHttpParamsFieldId(env, "inputFilePath")},
                  {"outputFilePath", GetHttpParamsFieldId(env, "outputFilePath")},
                  {"cookies", GetHttpParamsFieldId(env, "cookies")},
                  {"receivedUrl", GetHttpParamsFieldId(env, "receivedUrl")},
                  {"followRedirects", GetHttpParamsFieldId(env, "followRedirects", "Z")},
                  {"loadHeaders", GetHttpParamsFieldId(env, "loadHeaders", "Z")},
                  {"httpResponseCode", GetHttpParamsFieldId(env, "httpResponseCode", "I")},
                  {"timeoutMillisec", GetHttpParamsFieldId(env, "timeoutMillisec", "I")}};
  }

  jfieldID GetId(std::string const & fieldName) const
  {
    auto const it = m_fieldIds.find(fieldName);
    CHECK(it != m_fieldIds.end(), ("Incorrect field name:", fieldName));
    return it->second;
  }

private:
  std::unordered_map<std::string, jfieldID> m_fieldIds;
};

using CancelChecker = platform::HttpClient::CancelChecker;

// Context passed to Java as a native pointer, delivered back in nativeOnComplete.
struct AsyncContext
{
  platform::HttpClient::CompletionHandler m_handler;
  platform::HttpClient::ProgressHandler m_progressHandler;
  platform::HttpClient::DataHandler m_dataHandler;
  CancelChecker m_cancelChecker;
  // Global ref to the Params object so we can read results on the callback thread.
  jobject m_paramsGlobalRef = nullptr;
  std::atomic<bool> m_dataAborted{false};
};

// Extract Result from a Java Params object.
platform::HttpClient::Result ExtractResult(JNIEnv * env, jobject params)
{
  platform::HttpClient::Result result;
  ScopedEnv scopedEnv(jni::GetJVM());
  static Ids ids(scopedEnv);
  static jfieldID const responseDataField = env->GetFieldID(g_httpParamsClazz, "responseData", "[B");

  try
  {
    GetInt(scopedEnv, params, ids.GetId("httpResponseCode"), result.m_errorCode);
    GetString(scopedEnv, params, ids.GetId("receivedUrl"), result.m_urlReceived);

    platform::HttpClient::Headers headers;
    ::LoadHeaders(scopedEnv, params, headers);
    result.m_headers = std::move(headers);
  }
  catch (JniException const &)
  {
    return result;
  }

  jni::ScopedLocalRef<jbyteArray> const jniData(
      env, static_cast<jbyteArray>(env->GetObjectField(params, responseDataField)));
  if (!jni::HandleJavaException(env) && jniData)
  {
    jbyte * buffer = env->GetByteArrayElements(jniData.get(), nullptr);
    if (buffer)
    {
      result.m_serverResponse.assign(reinterpret_cast<char const *>(buffer), env->GetArrayLength(jniData.get()));
      env->ReleaseByteArrayElements(jniData.get(), buffer, JNI_ABORT);
    }
  }

  return result;
}
}  // namespace

// JNI streaming callbacks — called from Java during response body reading.

extern "C" JNIEXPORT jboolean Java_app_organicmaps_sdk_util_HttpClient_nativeOnData(JNIEnv * env, jclass /*clazz*/,
                                                                                    jlong nativeCtxPtr,
                                                                                    jbyteArray dataBuffer,
                                                                                    jint dataSize)
{
  auto * ctx = reinterpret_cast<AsyncContext *>(nativeCtxPtr);
  ASSERT(ctx && ctx->m_dataHandler, ());

  if (ctx->m_cancelChecker && ctx->m_cancelChecker())
    return JNI_FALSE;

  jbyte * buffer = env->GetByteArrayElements(dataBuffer, nullptr);
  if (!buffer)
    return JNI_FALSE;

  bool const shouldContinue = ctx->m_dataHandler(buffer, static_cast<size_t>(dataSize));
  env->ReleaseByteArrayElements(dataBuffer, buffer, JNI_ABORT);

  if (!shouldContinue)
    ctx->m_dataAborted.store(true, std::memory_order_release);

  return shouldContinue ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void Java_app_organicmaps_sdk_util_HttpClient_nativeOnProgress(JNIEnv * /*env*/, jclass /*clazz*/,
                                                                                    jlong nativeCtxPtr,
                                                                                    jlong bytesTransferred,
                                                                                    jlong totalBytes)
{
  auto * ctx = reinterpret_cast<AsyncContext *>(nativeCtxPtr);
  ASSERT(ctx && ctx->m_progressHandler, ());
  ctx->m_progressHandler(bytesTransferred, totalBytes);
}

extern "C" JNIEXPORT jboolean Java_app_organicmaps_sdk_util_HttpClient_hasNativeDataHandler(JNIEnv * /*env*/,
                                                                                            jclass /*clazz*/,
                                                                                            jlong nativeCtxPtr)
{
  auto * ctx = reinterpret_cast<AsyncContext *>(nativeCtxPtr);
  return (ctx && ctx->m_dataHandler) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean Java_app_organicmaps_sdk_util_HttpClient_hasNativeProgressHandler(JNIEnv * /*env*/,
                                                                                                jclass /*clazz*/,
                                                                                                jlong nativeCtxPtr)
{
  auto * ctx = reinterpret_cast<AsyncContext *>(nativeCtxPtr);
  return (ctx && ctx->m_progressHandler) ? JNI_TRUE : JNI_FALSE;
}

// JNI callback from OkHttp's thread — called by HttpClient.java after enqueue() completes.
extern "C" JNIEXPORT void Java_app_organicmaps_sdk_util_HttpClient_nativeOnComplete(JNIEnv * env, jclass /*clazz*/,
                                                                                    jlong nativeCtxPtr,
                                                                                    jboolean success)
{
  std::unique_ptr<AsyncContext> ctx(reinterpret_cast<AsyncContext *>(nativeCtxPtr));

  platform::HttpClient::Result result;

  // Check cancellation or data-handler abort.
  if ((ctx->m_cancelChecker && ctx->m_cancelChecker()) || ctx->m_dataAborted.load(std::memory_order_acquire))
  {
    result.m_errorCode = platform::HttpClient::kCancelled;
  }
  else if (success && ctx->m_paramsGlobalRef)
  {
    result = ExtractResult(env, ctx->m_paramsGlobalRef);
    result.m_success = true;
  }
  else if (ctx->m_paramsGlobalRef)
  {
    // Failure — extract error code from params.
    ScopedEnv scopedEnv(jni::GetJVM());
    static Ids ids(scopedEnv);
    try
    {
      GetInt(scopedEnv, ctx->m_paramsGlobalRef, ids.GetId("httpResponseCode"), result.m_errorCode);
    }
    catch (...)
    {}
  }

  // Clean up the global ref.
  if (ctx->m_paramsGlobalRef)
  {
    env->DeleteGlobalRef(ctx->m_paramsGlobalRef);
    ctx->m_paramsGlobalRef = nullptr;
  }

  if (ctx->m_handler)
    ctx->m_handler(std::move(result));
}

//***********************************************************************
// Exported functions implementation
//***********************************************************************
namespace platform
{
HttpClient::RequestHandle HttpClient::RunHttpRequestAsync(CompletionHandler handler)
{
  RequestHandle handle;
  handle.m_impl = std::make_shared<RequestHandle::Impl>();

  // Invoke handler with empty Result on any early return.
  // Released after handler ownership transfers to AsyncContext.
  SCOPE_GUARD(onError, [&handler]
  {
    if (handler)
      handler(Result{});
  });

  ScopedEnv env(jni::GetJVM());
  if (!env)
    return handle;

  static Ids ids(env);

  // Create and fill request params.
  jni::ScopedLocalRef<jstring> const jniUrl(env.get(), jni::ToJavaString(env.get(), m_urlRequested));
  if (jni::HandleJavaException(env.get()))
    return handle;

  static jmethodID const httpParamsConstructor =
      jni::GetConstructorID(env.get(), g_httpParamsClazz, "(Ljava/lang/String;)V");

  jni::ScopedLocalRef<jobject> const httpParamsObject(
      env.get(), env->NewObject(g_httpParamsClazz, httpParamsConstructor, jniUrl.get()));
  if (jni::HandleJavaException(env.get()))
    return handle;

  static jfieldID const requestDataField = env->GetFieldID(g_httpParamsClazz, "requestData", "[B");
  if (!m_bodyData.empty())
  {
    jni::ScopedLocalRef<jbyteArray> const jniPostData(env.get(),
                                                      env->NewByteArray(static_cast<jsize>(m_bodyData.size())));
    if (jni::HandleJavaException(env.get()))
      return handle;

    env->SetByteArrayRegion(jniPostData.get(), 0, static_cast<jsize>(m_bodyData.size()),
                            reinterpret_cast<jbyte const *>(m_bodyData.data()));
    if (jni::HandleJavaException(env.get()))
      return handle;

    env->SetObjectField(httpParamsObject.get(), requestDataField, jniPostData.get());
    if (jni::HandleJavaException(env.get()))
      return handle;
  }

  ASSERT(!m_httpMethod.empty(), ("Http method type can not be empty."));

  try
  {
    SetString(env, httpParamsObject.get(), ids.GetId("httpMethod"), m_httpMethod);
    SetString(env, httpParamsObject.get(), ids.GetId("inputFilePath"), m_inputFile);
    SetString(env, httpParamsObject.get(), ids.GetId("outputFilePath"), m_outputFile);
    SetString(env, httpParamsObject.get(), ids.GetId("cookies"), m_cookies);
    SetBoolean(env, httpParamsObject.get(), ids.GetId("followRedirects"), m_followRedirects);
    SetBoolean(env, httpParamsObject.get(), ids.GetId("loadHeaders"), m_loadHeaders);
    SetInt(env, httpParamsObject.get(), ids.GetId("timeoutMillisec"), static_cast<int>(m_timeoutSec * 1000));

    ::SetHeaders(env, httpParamsObject.get(), m_headers);
  }
  catch (JniException const &)
  {
    return handle;
  }

  // Allocate async context on the heap — will be freed in nativeOnComplete.
  auto ctx = std::make_unique<AsyncContext>();
  ctx->m_handler = std::move(handler);
  onError.release();
  ctx->m_progressHandler = m_progressHandler;
  ctx->m_dataHandler = m_dataHandler;
  ctx->m_cancelChecker = handle.MakeCancelChecker();
  ctx->m_paramsGlobalRef = env->NewGlobalRef(httpParamsObject.get());

  // Call Java HttpClient.runAsync(Params, long nativeCtxPtr) — returns a Call object.
  static jmethodID const runAsyncMethod = env->GetStaticMethodID(
      g_httpClientClazz, "runAsync", "(Lapp/organicmaps/sdk/util/HttpClient$Params;J)Lokhttp3/Call;");

  jlong const ctxPtr = reinterpret_cast<jlong>(ctx.get());
  jni::ScopedLocalRef<jobject> const jCall(
      env.get(), env->CallStaticObjectMethod(g_httpClientClazz, runAsyncMethod, httpParamsObject.get(), ctxPtr));

  if (jni::HandleJavaException(env.get()))
  {
    // nativeOnComplete was NOT called (uncaught Java exception) — ctx is still
    // owned by unique_ptr. Clean up and invoke handler to avoid deadlock in sync wrapper.
    env->DeleteGlobalRef(ctx->m_paramsGlobalRef);
    ctx->m_paramsGlobalRef = nullptr;
    if (ctx->m_handler)
      ctx->m_handler(Result{});
    return handle;
  }

  // Context ownership transferred to Java callback — release from unique_ptr.
  ctx.release();

  // Store OkHttp Call for cancellation.
  // Use shared_ptr with custom deleter to ensure the global ref is freed exactly once,
  // whether the request completes normally (Impl destroyed) or is cancelled.
  if (jCall)
  {
    auto callRef = std::shared_ptr<_jobject>(env->NewGlobalRef(jCall.get()), [](jobject ref)
    {
      ScopedEnv scopedEnv(jni::GetJVM());
      scopedEnv.get()->DeleteGlobalRef(ref);
    });

    // Use GetObjectClass instead of FindClass to avoid class loader issues:
    // native threads attached via ScopedEnv use the system class loader,
    // which can't find app/library classes like okhttp3.Call.
    jclass const callClass = env->GetObjectClass(jCall.get());
    jmethodID const cancelMethodId = env->GetMethodID(callClass, "cancel", "()V");
    env->DeleteLocalRef(callClass);

    std::lock_guard lock(handle.m_impl->m_mu);
    handle.m_impl->m_platformCancel = [callRef, cancelMethodId]
    {
      ScopedEnv scopedEnv(jni::GetJVM());
      scopedEnv.get()->CallVoidMethod(callRef.get(), cancelMethodId);
    };
  }

  return handle;
}
}  // namespace platform
