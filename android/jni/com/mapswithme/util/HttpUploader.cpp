#include <jni.h>

#include "com/mapswithme/core/ScopedEnv.hpp"
#include "com/mapswithme/core/ScopedLocalRef.hpp"
#include "com/mapswithme/core/jni_helper.hpp"

#include "platform/http_uploader.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <functional>

#include "HttpUploaderUtils.hpp"
#include "private.h"

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

  HttpPayload const payload = GetPayload();
  jobject uploader = platform::MakeHttpUploader(env, payload, g_httpUploaderClazz);
  jni::ScopedLocalRef<jobject> const uploaderRef(env, uploader);

  static jmethodID const uploadId = jni::GetMethodID(env, uploaderRef.get(), "upload",
                                                     "()Lcom/mapswithme/util/HttpUploader$Result;");

  jni::ScopedLocalRef<jobject> const result(env,
                                            env->CallObjectMethod(uploaderRef.get(), uploadId));

  if (jni::HandleJavaException(env))
  {
    Result invalidResult;
    invalidResult.m_httpCode = -1;
    invalidResult.m_description = "Unhandled exception during upload is encountered!";
    return invalidResult;
  }

  return ToNativeResult(env, result);
}
}  // namespace platform

extern "C"
{
  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_util_HttpUploader_nativeUserBindingCertificate(JNIEnv * env, jclass)
  {
    return jni::ToJavaString(env, USER_BINDING_PKCS12);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_util_HttpUploader_nativeUserBindingPassword(JNIEnv * env, jclass)
  {
    return jni::ToJavaString(env, USER_BINDING_PKCS12_PASSWORD);
  }
}
