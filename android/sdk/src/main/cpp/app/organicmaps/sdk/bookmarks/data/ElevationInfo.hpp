#pragma once

#include <jni.h>

#include "geometry/latlon.hpp"
#include "map/elevation_info.hpp"

jobject ToJavaElevationInfoPoint(JNIEnv * env, ms::LatLon const & latlon);

jobject ToJavaElevationInfo(JNIEnv * env, ElevationInfo const & info);
