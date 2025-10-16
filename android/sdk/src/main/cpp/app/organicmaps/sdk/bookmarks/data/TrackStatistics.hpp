#pragma once

#include <jni.h>

#include "map/track_statistics.hpp"

jobject ToJavaTrackStatistics(JNIEnv * env, TrackStatistics const & stats);
