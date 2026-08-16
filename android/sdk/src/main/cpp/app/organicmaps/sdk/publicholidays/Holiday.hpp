#pragma once

#include <jni.h>

#include "public_holidays/holidays.hpp"

jobject ToJavaHoliday(JNIEnv * env, om::public_holidays::Holiday holiday);
