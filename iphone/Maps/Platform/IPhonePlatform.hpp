#pragma once
#include "../../platform/platform.hpp"

#import <Foundation/NSDate.h>

class IPhonePlatform : public Platform
{
public:
  IPhonePlatform();
  virtual ~IPhonePlatform() {}
  virtual double TimeInSec();
  virtual string WorkingDir();
  virtual string ResourcesDir();
  virtual int GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles);
  virtual bool GetFileSize(string const & file, uint64_t & size);  
  virtual bool RenameFileX(string const & original, string const & newName);
  virtual int CpuCores();
	virtual double VisualScale() const;
	virtual string const SkinName() const;
	virtual bool IsMultiSampled() const;
  
private:
	string m_skinName;
	double m_visualScale;
  NSDate * m_StartDate;
	bool m_isMultiSampled;
};
