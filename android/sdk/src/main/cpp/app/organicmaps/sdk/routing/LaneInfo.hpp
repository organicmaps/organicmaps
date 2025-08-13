#pragma once

#include <jni.h>

#include "routing/lanes/lane_info.hpp"

jobjectArray CreateLanesInfo(JNIEnv * env, routing::turns::lanes::LanesInfo const & lanes);
