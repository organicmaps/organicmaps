#pragma once

#include <jni.h>

enum class RoadWarningMarkType : uint8_t;

jobject CreateRoadWarningMarkType(JNIEnv * env, RoadWarningMarkType roadWarningMarkType);
