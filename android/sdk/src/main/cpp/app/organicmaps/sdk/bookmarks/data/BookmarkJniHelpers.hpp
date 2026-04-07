#pragma once

#include <jni.h>

class Bookmark;
class Track;

namespace bookmark_jni
{
void PrepareClassRefs(JNIEnv * env);
jobject CreateBookmarkInfo(JNIEnv * env, Bookmark const & bookmark);
jobject CreateTrack(JNIEnv * env, Track const & track);
}  // namespace bookmark_jni
