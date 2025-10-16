#pragma once

#include <jni.h>

#include "indexer/feature_decl.hpp"

jobject CreateFeatureId(JNIEnv * env, FeatureID const & fid);
jobjectArray ToFeatureIdArray(JNIEnv * env, std::vector<FeatureID> const & ids);
