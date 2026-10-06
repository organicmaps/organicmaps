#pragma once

#include <jni.h>

namespace place_page
{
enum class OpeningMode;
}

jobject CreateOpeningMode(JNIEnv * env, place_page::OpeningMode openingMode);
