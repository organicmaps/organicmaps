#pragma once

#include "../../platform/platform.hpp"

#include <jni.h>

class AndroidPlatform : public BasePlatformImpl
{
public:
  void Initialize(JNIEnv * env, jobject activity, jstring path);

  virtual void GetFontNames(FilesList & res) const;
  virtual int CpuCores() const;
  virtual string DeviceID() const;
};

AndroidPlatform & GetAndroidPlatform();
