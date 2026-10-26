#pragma once

#include <jni.h>

#include "map/elevation_info.hpp"

#include <functional>

jobject ToJavaElevationInfo(JNIEnv * env, ElevationInfo const & info, std::function<bool()> const & isCancelled = {});
