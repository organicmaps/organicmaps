#pragma once

#include <jni.h>

class Bookmark;

jobject CreateBookmarkInfo(JNIEnv * env, Bookmark const & bookmark);
