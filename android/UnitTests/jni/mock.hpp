#pragma once

#include <android/log.h>
#include <android_native_app_glue.h>
#include <cassert>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

void Initialize(android_app * state);
JavaVM * GetJVM();
