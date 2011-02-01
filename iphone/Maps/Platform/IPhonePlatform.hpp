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
	virtual vector<string> GetFontNames() const;
  
private:
	string m_skinName;
	double m_visualScale;
  NSDate * m_StartDate;
	bool m_isMultiSampled;
	bool m_doPeriodicalUpdate;
  string m_resourcesPath;
  string m_writablePath;
};
