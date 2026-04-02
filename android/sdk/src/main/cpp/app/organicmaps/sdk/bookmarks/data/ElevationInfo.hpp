#pragma once

#include <jni.h>

#include "map/elevation_info.hpp"

jobject ToJavaElevationInfo(JNIEnv * env, ElevationInfo const & info);
