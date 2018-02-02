#include <jni.h>

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/core/ScopedEnv.hpp"
#include "com/mapswithme/core/ScopedLocalRef.hpp"

#include "platform/http_uploader.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <functional>

namespace
{
platform::HttpUploader::Result ToNativeResult(JNIEnv * env, jobject const src)
{
  static jmethodID const getHttpCode =
    env->GetMethodID(g_httpUploaderResultClazz, "getHttpCode", "()I");
  static jmethodID const getDescription =
    env->GetMethodID(g_httpUploaderResultClazz, "getDescription", "()Ljava/lang/String;");

  platform::HttpUploader::Result result;

  result.m_httpCode = static_cast<int32_t>(env->CallIntMethod(src, getHttpCode));

  jni::ScopedLocalRef<jstring> const description(
    env, static_cast<jstring>(env->CallObjectMethod(src, getDescription)));
  result.m_description = jni::ToNativeString(env, description.get());

  return result;
}
}  // namespace

namespace platform
{
HttpUploader::Result HttpUploader::Upload() const
{
  auto env = jni::GetEnv();

  CHECK(env, ());

  static jmethodID const httpUploaderConstructor =
    jni::GetConstructorID(env, g_httpUploaderClazz, "(Ljava/lang/String;Ljava/lang/String;"
                                                    "[Lcom/mapswithme/util/KeyValue;"
                                                    "[Lcom/mapswithme/util/KeyValue;"
                                                    "Ljava/lang/String;Ljava/lang/String;)V");

  jni::ScopedLocalRef<jstring> const method(env, jni::ToJavaString(env, m_method));
  jni::ScopedLocalRef<jstring> const url(env, jni::ToJavaString(env, m_url));
  jni::ScopedLocalRef<jobjectArray> const params(env, jni::ToKeyValueArray(env, m_params));
  jni::ScopedLocalRef<jobjectArray> const headers(env, jni::ToKeyValueArray(env, m_headers));
  jni::ScopedLocalRef<jstring> const fileKey(env, jni::ToJavaString(env, m_fileKey));
  jni::ScopedLocalRef<jstring> const filePath(env, jni::ToJavaString(env, m_filePath));

  jni::ScopedLocalRef<jobject> const httpUploaderObject(
    env, env->NewObject(g_httpUploaderClazz, httpUploaderConstructor, method.get(), url.get(),
                        params.get(), headers.get(), fileKey.get(), filePath.get()));

  static jmethodID const uploadId = jni::GetMethodID(env, httpUploaderObject, "upload",
                                                     "()Lcom/mapswithme/util/HttpUploader$Result;");

  jni::ScopedLocalRef<jobject> const result(
    env, env->CallObjectMethod(httpUploaderObject, uploadId));

  return ToNativeResult(env, result);
}
} // namespace platform
