#pragma once

#include <jni.h>

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "routing/turns.hpp"

jobject ToJavaCarDirection(JNIEnv * env, routing::turns::CarDirection turn);
