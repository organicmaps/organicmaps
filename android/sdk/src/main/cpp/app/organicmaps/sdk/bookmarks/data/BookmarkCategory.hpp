#pragma once

#include <jni.h>

#include "kml/type_utils.hpp"

#include <optional>

jobject ToJavaBookmarkCategory(JNIEnv * env, kml::MarkGroupId id);

jobjectArray ToJavaBookmarkCategories(JNIEnv * env, kml::GroupIdCollection const & ids);

// Returns the default color index for a category, or std::nullopt if not set or invalid.
std::optional<size_t> GetCategoryDefaultColorIndex(kml::MarkGroupId catId);
