#include "coding/zip_creator.hpp"

#include "base/string_utils.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"
#include "coding/constants.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "std/vector.hpp"
#include "std/ctime.hpp"
#include "std/algorithm.hpp"

#include "3party/minizip/zip.h"


namespace
{

class ZipHandle
{
  zipFile m_zipFileHandle;

public:
  ZipHandle(string const & filePath)
  {
    m_zipFileHandle = zipOpen(filePath.c_str(), 0);
  }

  ~ZipHandle()
  {
    if (m_zipFileHandle)
      zipClose(m_zipFileHandle, NULL);
  }

  zipFile Handle() const { return m_zipFileHandle; }
};

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

bool CreateZipFromPathDeflatedAndDefaultCompression(string const & filePath, string const & zipFilePath)
{
  // 2. Open zip file for writing.
  MY_SCOPE_GUARD(outFileGuard, bind(&my::DeleteFileX, cref(zipFilePath)));
  ZipHandle zip(zipFilePath);
  if (!zip.Handle())
    return false;

  zip_fileinfo zipInfo = {};
  CreateTMZip(zipInfo.tmz_date);

  string fileName = filePath;
  my::GetNameFromFullPath(fileName);
  if (!strings::IsASCIIString(fileName))
    fileName = "MapsMe.kml";

  if (zipOpenNewFileInZip(zip.Handle(), fileName.c_str(), &zipInfo,
                          NULL, 0, NULL, 0, "ZIP from MapsWithMe", Z_DEFLATED, Z_DEFAULT_COMPRESSION) < 0)
  {
    return false;
  }

  // Write source file into zip file.
  try
  {
    my::FileData file(filePath, my::FileData::OP_READ);
    uint64_t const fileSize = file.Size();

    uint64_t currSize = 0;
    char buffer[ZIP_FILE_BUFFER_SIZE];
    while (currSize < fileSize)
    {
      unsigned int const toRead = min(ZIP_FILE_BUFFER_SIZE, static_cast<unsigned int>(fileSize - currSize));
      file.Read(currSize, buffer, toRead);

      if (ZIP_OK != zipWriteInFileInZip(zip.Handle(), buffer, toRead))
        return false;

      currSize += toRead;
    }
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LERROR, ("Error reading file:", filePath, ex.Msg()));
    return false;
  }

  // Success.
  outFileGuard.release();
  return true;
}
