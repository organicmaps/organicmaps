#pragma once
#include "jni.h"
#include "platform/http_payload.hpp"

namespace platform
{
jobject MakeHttpUploader(JNIEnv * env, const platform::HttpPayload payload, jclass uploaderClass);
}
