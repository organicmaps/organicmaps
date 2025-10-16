#pragma once

#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "map/place_page_info.hpp"

jobject CreateTrack(JNIEnv * env, place_page::Info const & info, jni::TScopedLocalObjectArrayRef const & jrawTypes,
                    jni::TScopedLocalRef const & routingPointInfo);
