#pragma once

#include <jni.h>

namespace place_page
{
class Info;
}  // namespace place_page

// TODO(yunikkk): this helper is redundant with new place page info approach.
// It's better to refactor MapObject in Java, may be removing it at all, and to make simple jni getters for
// globally stored place_page::Info object. Code should be clean and easy to support in this way.
jobject CreateMapObject(JNIEnv * env, place_page::Info const & info);
