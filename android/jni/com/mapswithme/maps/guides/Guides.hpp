#pragma once

#include "com/mapswithme/core/jni_helper.hpp"
#include "map/guides_manager.hpp"

namespace guides
{
jobject CreateGallery(JNIEnv * env, GuidesManager::GuidesGallery const & gallery);
} // namespace

namespace platform
{
bool IsGuidesLayerFirstLaunch();
void SetGuidesLayerFirstLaunch(bool isFirstLaunch);
}  // namespace platform
