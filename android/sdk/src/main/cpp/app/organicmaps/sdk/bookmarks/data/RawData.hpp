#pragma once

#include <jni.h>

#include <string>
#include <vector>

jobject CreateRawData(JNIEnv * env, std::vector<std::string> const & rawData);
