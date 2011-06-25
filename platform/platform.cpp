#include "platform.hpp"

#include "../coding/internal/file_data.hpp"

#include "../base/logging.hpp"

#include "../base/start_mem_debug.hpp"


string BasePlatformImpl::ReadPathForFile(string const & file) const
{
  string fullPath = m_writableDir + file;
  if (!IsFileExists(fullPath))
  {
    fullPath = m_resourcesDir + file;
    if (!IsFileExists(fullPath))
      MYTHROW(FileAbsentException, ("File doesn't exist", fullPath));
  }
  return fullPath;
}

bool BasePlatformImpl::GetFileSize(string const & file, uint64_t & size) const
{
  return my::GetFileSize(file, size);
}

void BasePlatformImpl::GetFontNames(FilesList & res) const
{
  res.clear();
  GetFilesInDir(m_resourcesDir, "*.ttf", res);

  sort(res.begin(), res.end());

  for (size_t i = 0; i < res.size(); ++i)
    res[i] = m_resourcesDir + res[i];
}

double BasePlatformImpl::VisualScale() const
{
  return 1.0;
}

string BasePlatformImpl::SkinName() const
{
  return "basic.skn";
}

bool BasePlatformImpl::IsMultiSampled() const
{
  return true;
}

bool BasePlatformImpl::DoPeriodicalUpdate() const
{
  return true;
}

double BasePlatformImpl::PeriodicalUpdateInterval() const
{
  return 0.3;
}

bool BasePlatformImpl::IsBenchmarking() const
{
  bool res = false;
#ifndef OMIM_PRODUCTION
  if (res)
  {
    static bool first = true;
    if (first)
    {
      LOG(LCRITICAL, ("benchmarking only defined in production configuration"));
      first = false;
    }
    res = false;
  }
#endif
  return res;
}

bool BasePlatformImpl::IsVisualLog() const
{
  return false;
}

int BasePlatformImpl::ScaleEtalonSize() const
{
  return 512 + 256;
}
