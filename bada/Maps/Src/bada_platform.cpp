#include "../../../platform/platform.hpp"
#include "../../../coding/strutil.hpp"

#include <FBaseTimeSpan.h>
#include <FSysSystemTime.h>
#include <FIoFile.h>
#include <FIoDirectory.h>

using namespace Osp::Base;
using namespace Osp::System;
using namespace Osp::Io;

/// Converts bad(a) strings to good utf8 strings
std::string BadaStringToStdString(String const & badaStr)
{
  mchar wideChar;
  wstring unicodeString;
  for (int i = 0; i < badaStr.GetLength(); ++i)
  {
    badaStr.GetCharAt(i, wideChar);
    unicodeString.push_back(wideChar);
  }
  return ToUtf8(unicodeString);
}

class BadaPlatform : public Platform
{
  TimeSpan m_startTime;

public:
  BadaPlatform() : m_startTime(0)
  {
    SystemTime::GetUptime(m_startTime);
  }

  virtual double TimeInSec()
  {
    TimeSpan currentTime(0);
    SystemTime::GetUptime(currentTime);
    return static_cast<double>((currentTime - m_startTime).GetTicks()) /
        static_cast<double>(TimeSpan::NUM_OF_TICKS_IN_SECOND);
  }

  virtual string WorkingDir()
  {
    return "/Home/";
  }

  virtual string ResourcesDir()
  {
    return "/Res/";
  }

  /// @NOTE current implementation doesn't support wildcard in mask, so "*" symbol will be ignored and
  /// simple substring comparison will be made
  virtual int GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles)
  {
    // open directory
    Directory dir;
    if (IsFailed(dir.Construct(directory.c_str())))
      return 0;

    // read all directory entries
    DirEnumerator * pDirEnum = dir.ReadN();
    if (!pDirEnum)
      return 0;

    // temporary workaround for wildcards
    string fixedMask(mask);
    if (fixedMask.size() && fixedMask[0] == '*')
      fixedMask.erase(0, 1);

    // loop through all directory entries
    while (pDirEnum->MoveNext() == E_SUCCESS)
    {
       DirEntry dirEntry = pDirEnum->GetCurrentDirEntry();
       String file = dirEntry.GetName();
       if (file.EndsWith(fixedMask.c_str()))
         outFiles.push_back(BadaStringToStdString(file));
    }

    // delete enumerator
    delete pDirEnum;
    return outFiles.size();
  }

  virtual bool GetFileSize(string const & file, uint64_t & size)
  {
    FileAttributes attr;
    result error = File::GetAttributes(file.c_str(), attr);
    if (IsFailed(error))
      return false;
    size = attr.GetFileSize();
    return true;
  }

  virtual bool RenameFileX(string const & original, string const & newName)
  {
    return File::Move(original.c_str(), newName.c_str()) == E_SUCCESS;
  }
};

Platform & GetPlatform()
{
  static BadaPlatform pl;
  return pl;
}
