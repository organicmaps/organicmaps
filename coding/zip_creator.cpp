#include "zip_creator.hpp"

#include "../../coding/internal/file_data.hpp"

#include "../../std/vector.hpp"
#include "../../std/iostream.hpp"
#include "../../std/ctime.hpp"

#include "../../3party/zlib/contrib/minizip/zip.h"


class ZipHandle
{
public:
  zipFile m_zipFile;
  ZipHandle(string const & filePath)
  {
    m_zipFile = zipOpen(filePath.c_str(), 0);
  }
  ~ZipHandle()
  {
    if (m_zipFile)
      zipClose(m_zipFile, NULL);
  }
};

namespace
{
void CreateTMZip(tm_zip & res)
{
  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  res.tm_sec = timeinfo->tm_sec;
  res.tm_min = timeinfo->tm_min;
  res.tm_hour = timeinfo->tm_hour;
  res.tm_mday = timeinfo->tm_mday;
  res.tm_mon = timeinfo->tm_mon;
  res.tm_year = timeinfo->tm_year;
}
}

bool createZipFromPathDeflatedAndDefaultCompression(string const & filePath, string const & zipFilePath)
{
  ZipHandle zip(zipFilePath);
  if (!zip.m_zipFile)
    return false;
  zip_fileinfo zipInfo = { 0 };
  CreateTMZip(zipInfo.tmz_date);
  if (zipOpenNewFileInZip (zip.m_zipFile, filePath.c_str(), &zipInfo, NULL, 0, NULL, 0, "ZIP from MapsWithMe", Z_DEFLATED, Z_DEFAULT_COMPRESSION) < 0)
    return false;

  my::FileData f(filePath, my::FileData::OP_READ);

  size_t const bufSize = 512 * 1024;
  vector<char> buffer(bufSize);
  size_t const fileSize = f.Size();
  size_t currSize = 0;

  while (currSize < fileSize)
  {
    size_t toRead = fileSize - currSize;
    if (toRead > bufSize)
      toRead = bufSize;
    f.Read(currSize, &buffer[0], toRead);
    if (ZIP_OK != zipWriteInFileInZip (zip.m_zipFile, &buffer[0], toRead))
      return false;
    currSize += toRead;
  }
  return true;
}
