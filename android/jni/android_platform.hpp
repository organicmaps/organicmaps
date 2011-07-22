#pragma once

#include "../../platform/platform.hpp"

#include <jni.h>

class AndroidPlatform : public BasePlatformImpl
{
public:
  void Initialize(JNIEnv * env, jobject activity, jstring apkPath, jstring storagePath);

  virtual ModelReader * GetReader(string const & file) const;
  /// Overrided to support zip file listing
  virtual void GetFilesInDir(string const & directory, string const & mask, FilesList & res) const;

  virtual int CpuCores() const;
  virtual string DeviceID() const;

  double VisualScale() const;
  string SkinName() const;
};

AndroidPlatform & GetAndroidPlatform();
