#include <jni.h>

#include "com/mapswithme/core/ScopedEnv.hpp"
#include "com/mapswithme/core/ScopedLocalRef.hpp"
#include "com/mapswithme/core/jni_helper.hpp"

#include "platform/http_uploader.hpp"
#include "platform/http_uploader_background.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <functional>

#include "HttpUploaderUtils.hpp"
#include "private.h"

namespace platform
{
void HttpUploaderBackground::Upload() const
{
  JNIEnv * env = jni::GetEnv();

  CHECK(env, ());

  HttpPayload const payload = GetPayload();
  jobject uploader = platform::MakeHttpUploader(env, payload, g_httpBackgroundUploaderClazz);
  jni::TScopedLocalRef uploaderRef(env, uploader);

  static jmethodID const uploadId = jni::GetMethodID(env, uploaderRef.get(), "upload", "()V");
  env->CallVoidMethod(uploaderRef.get(), uploadId);
  jni::HandleJavaException(env);
}
}  // namespace platform
