#pragma once
#include "../../platform/platform.hpp"


class IPhonePlatform : public BasePlatformImpl
{
public:
  IPhonePlatform();
  virtual ~IPhonePlatform();

  virtual int CpuCores() const;
  virtual double VisualScale() const;
  virtual bool IsMultiSampled() const;
  virtual string SkinName() const;
  virtual string DeviceID() const;
  virtual int ScaleEtalonSize() const;

private:
	string m_deviceID;
	string m_skinName;
	double m_visualScale;
	unsigned m_scaleEtalonSize;
};
