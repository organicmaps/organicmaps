#pragma once

#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "map/place_page_info.hpp"

class Track;

jobject CreateTrack(JNIEnv * env, place_page::Info const & info, jni::TScopedLocalObjectArrayRef const & jrawTypes,
                    jni::TScopedLocalRef const & routingPointInfo);
jobject CreateTrack(JNIEnv * env, Track const & track);
