#pragma once

#include <jni.h>

#include "map/bookmark.hpp"

jobject CreateBookmark(JNIEnv * env, place_page::Info const & info, jni::TScopedLocalObjectArrayRef const & jrawTypes,
                       jni::TScopedLocalRef const & routingPointInfo);

Bookmark const * getBookmark(jlong bokmarkId);
