#pragma once

#include <jni.h>

jobject CreateApiData(JNIEnv * env, std::string const & id, std::string const & url);
