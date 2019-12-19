#include <jni.h>

#include "com/mapswithme/core/ScopedEnv.hpp"
#include "com/mapswithme/core/ScopedLocalRef.hpp"
#include "com/mapswithme/core/jni_helper.hpp"

#include "platform/http_uploader.hpp"

namespace platform
{
jobject ToJavaHttpPayload(JNIEnv *env, const platform::HttpPayload payload)
{
  static jmethodID const constructor =
      jni::GetConstructorID(env, g_httpPayloadClazz,
                            "(Ljava/lang/String;Ljava/lang/String;"
                            "[Lcom/mapswithme/util/KeyValue;"
                            "[Lcom/mapswithme/util/KeyValue;"
                            "Ljava/lang/String;Ljava/lang/String;Z)V");

  jni::ScopedLocalRef<jstring> const method(env, jni::ToJavaString(env, payload.m_method));
  jni::ScopedLocalRef<jstring> const url(env, jni::ToJavaString(env, payload.m_url));
  jni::ScopedLocalRef<jobjectArray> const params(env, jni::ToKeyValueArray(env, payload.m_params));
  jni::ScopedLocalRef<jobjectArray> const headers(env,
                                                  jni::ToKeyValueArray(env, payload.m_headers));
  jni::ScopedLocalRef<jstring> const fileKey(env, jni::ToJavaString(env, payload.m_fileKey));
  jni::ScopedLocalRef<jstring> const filePath(env, jni::ToJavaString(env, payload.m_filePath));

  return env->NewObject(g_httpPayloadClazz, constructor, method.get(), url.get(), params.get(),
                        headers.get(), fileKey.get(), filePath.get(),
                        static_cast<jboolean>(payload.m_needClientAuth));
}

jobject MakeHttpUploader(JNIEnv * env, const platform::HttpPayload payload, jclass uploaderClass)
{
  static jmethodID const httpUploaderConstructor =
      jni::GetConstructorID(env, uploaderClass, "(Lcom/mapswithme/util/HttpPayload;)V");

  jni::TScopedLocalRef javaPayloadRef(env, ToJavaHttpPayload(env, payload));

  return env->NewObject(uploaderClass, httpUploaderConstructor, javaPayloadRef.get());
}
}  // namespace platform
