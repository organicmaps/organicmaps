#pragma once

#include <jni.h>

#include "../../../../../platform/platform.hpp"

namespace android
{
  class Platform : public ::Platform
  {
  public:
    void Initialize(JNIEnv * env, jstring apkPath, jstring storagePath);

    static Platform & Instance();
  };
}
