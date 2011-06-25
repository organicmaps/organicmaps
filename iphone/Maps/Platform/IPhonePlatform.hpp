#pragma once
#include "../../platform/platform.hpp"


class IPhonePlatform : public BasePlatformImpl
{
public:
  IPhonePlatform();
  virtual ~IPhonePlatform();

  virtual void GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles) const;
  virtual bool RenameFileX(string const & original, string const & newName) const;
  virtual int CpuCores() const;
  virtual double VisualScale() const;
  virtual string SkinName() const;
  virtual string DeviceID() const;
  virtual int ScaleEtalonSize() const;

private:
	string m_deviceID;
	string m_skinName;
	double m_visualScale;
	unsigned m_scaleEtalonSize;
};
