#pragma once

#include <jni.h>

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "map/bookmark.hpp"

jobject CreateBookmark(JNIEnv * env, place_page::Info const & info);

Bookmark const * getBookmark(jlong bokmarkId);
