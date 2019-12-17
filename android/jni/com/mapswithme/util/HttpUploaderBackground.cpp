#include <jni.h>

#include "com/mapswithme/core/ScopedEnv.hpp"
#include "com/mapswithme/core/ScopedLocalRef.hpp"
#include "com/mapswithme/core/jni_helper.hpp"

#include "platform/http_uploader_background.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <functional>

#include "private.h"

namespace platform
{
void HttpUploaderBackground::Upload() const
{
  JNIEnv* env = jni::GetEnv();

  CHECK(env, ());

  static jmethodID const httpUploaderConstructor =
      jni::GetConstructorID(env, g_httpUploaderBackgroundClazz,
                            "(Ljava/lang/String;Ljava/lang/String;"
                            "[Lcom/mapswithme/util/KeyValue;"
                            "[Lcom/mapswithme/util/KeyValue;"
                            "Ljava/lang/String;Ljava/lang/String;Z)V");
  HttpPayload const payload = GetPayload();
  jni::ScopedLocalRef<jstring> const method(env, jni::ToJavaString(env, payload.m_method));
  jni::ScopedLocalRef<jstring> const url(env, jni::ToJavaString(env, payload.m_url));
  jni::ScopedLocalRef<jobjectArray> const params(env, jni::ToKeyValueArray(env, payload.m_params));
  jni::ScopedLocalRef<jobjectArray> const headers(env,
                                                  jni::ToKeyValueArray(env, payload.m_headers));
  jni::ScopedLocalRef<jstring> const fileKey(env, jni::ToJavaString(env, payload.m_fileKey));
  jni::ScopedLocalRef<jstring> const filePath(env, jni::ToJavaString(env, payload.m_filePath));

  jni::ScopedLocalRef<jobject> const httpUploaderObject(
      env, env->NewObject(g_httpUploaderBackgroundClazz, httpUploaderConstructor, method.get(), url.get(),
                          params.get(), headers.get(), fileKey.get(), filePath.get(),
                          static_cast<jboolean>(payload.m_needClientAuth)));

  static jmethodID const uploadId = jni::GetMethodID(env, httpUploaderObject, "upload", "()V");
   env->CallVoidMethod(httpUploaderObject, uploadId);
}
}  // namespace platform

extern "C"
{
}
