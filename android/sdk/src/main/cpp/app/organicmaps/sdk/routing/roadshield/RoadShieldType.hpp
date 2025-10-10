#pragma once

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "indexer/road_shields_parser.hpp"

jobject ToJavaRoadShieldType(JNIEnv * env, ftypes::RoadShieldType roadShieldType);
