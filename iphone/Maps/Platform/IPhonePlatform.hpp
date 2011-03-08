#pragma once
#include "../../platform/platform.hpp"

#import <Foundation/NSDate.h>

class IPhonePlatform : public Platform
{
public:
  IPhonePlatform();
  virtual ~IPhonePlatform() {}
  virtual double TimeInSec() const;
  virtual string WritableDir() const;
  virtual string ReadPathForFile(char const * file) const;
  virtual int GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles) const;
  virtual bool GetFileSize(string const & file, uint64_t & size) const;
  virtual bool RenameFileX(string const & original, string const & newName) const;
  virtual int CpuCores() const;
	virtual double VisualScale() const;
	virtual string const SkinName() const;
	virtual bool IsMultiSampled() const;
	virtual bool DoPeriodicalUpdate() const;
	virtual double PeriodicalUpdateInterval() const;
	virtual vector<string> GetFontNames() const;
	virtual bool IsBenchmarking() const;
	virtual bool IsVisualLog() const;
  virtual string const DeviceID() const;
	virtual unsigned ScaleEtalonSize() const;
	
private:
	string m_deviceID;
	string m_skinName;
	double m_visualScale;
  NSDate * m_StartDate;
	bool m_isMultiSampled;
	bool m_doPeriodicalUpdate;
	double m_periodicalUpdateInterval;
  string m_resourcesPath;
  string m_writablePath;
	unsigned m_scaleEtalonSize;
};
