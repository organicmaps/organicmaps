#pragma once

#include "../../platform/platform.hpp"

#include <jni.h>

class AndroidPlatform : public Platform
{
public:
  void Initialize(JNIEnv * env, jstring apkPath, jstring storagePath);
};

AndroidPlatform & GetAndroidPlatform();
