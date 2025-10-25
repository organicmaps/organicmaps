#pragma once

#include <jni.h>

#include "kml/type_utils.hpp"

jobject ToJavaBookmarkCategory(JNIEnv * env, kml::MarkGroupId id);

jobjectArray ToJavaBookmarkCategories(JNIEnv * env, kml::GroupIdCollection const & ids);
