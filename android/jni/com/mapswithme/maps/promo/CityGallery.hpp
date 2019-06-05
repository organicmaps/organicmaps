#pragma once

#include "com/mapswithme/core/jni_helper.hpp"

namespace promo
{
struct CityGallery;

jobject MakeCityGallery(JNIEnv * env, promo::CityGallery const & gallery);
}  // namespace promo
