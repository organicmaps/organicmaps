#include "coding/zip_creator.hpp"

#include "base/string_utils.hpp"

#include "coding/constants.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "3party/minizip/zip.h"

#include <algorithm>
#include <array>
#include <ctime>
#include <exception>
#include <vector>

namespace
{
class ZipHandle
{
  zipFile m_zipFileHandle;

public:
  explicit ZipHandle(std::string const & filePath)
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
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  res.tm_sec = timeinfo->tm_sec;
  res.tm_min = timeinfo->tm_min;
  res.tm_hour = timeinfo->tm_hour;
  res.tm_mday = timeinfo->tm_mday;
  res.tm_mon = timeinfo->tm_mon;
  res.tm_year = timeinfo->tm_year;
}

int GetCompressionLevel(CompressionLevel compression)
{
  switch (compression)
  {
  case CompressionLevel::NoCompression: return Z_NO_COMPRESSION;
  case CompressionLevel::BestSpeed: return Z_BEST_SPEED;
  case CompressionLevel::BestCompression: return Z_BEST_COMPRESSION;
  case CompressionLevel::DefaultCompression: return Z_DEFAULT_COMPRESSION;
  case CompressionLevel::Count: UNREACHABLE();
  }
  UNREACHABLE();
}
}  // namespace

bool CreateZipFromPathDeflatedAndDefaultCompression(std::string const & filePath,
                                                    std::string const & zipFilePath)
{
  // Open zip file for writing.
  SCOPE_GUARD(outFileGuard, [&zipFilePath]() { base::DeleteFileX(zipFilePath); });

  ZipHandle zip(zipFilePath);
  if (!zip.Handle())
    return false;

  zip_fileinfo zipInfo = {};
  CreateTMZip(zipInfo.tmz_date);

  std::string fileName = filePath;
  base::GetNameFromFullPath(fileName);
  if (!strings::IsASCIIString(fileName))
    fileName = "MapsMe.kml";

  if (zipOpenNewFileInZip(zip.Handle(), fileName.c_str(), &zipInfo, nullptr, 0, nullptr, 0,
                          "ZIP from MapsWithMe", Z_DEFLATED, Z_DEFAULT_COMPRESSION) < 0)
  {
    return false;
  }

  // Write source file into zip file.
  try
  {
    base::FileData file(filePath, base::FileData::OP_READ);
    uint64_t const fileSize = file.Size();

    uint64_t currSize = 0;
    char buffer[ZIP_FILE_BUFFER_SIZE];
    while (currSize < fileSize)
    {
      unsigned int const toRead =
          std::min(ZIP_FILE_BUFFER_SIZE, static_cast<unsigned int>(fileSize - currSize));
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

  outFileGuard.release();
  return true;
}

bool CreateZipFromFiles(std::vector<std::string> const & files, std::string const & zipFilePath,
                        CompressionLevel compression)
{
  SCOPE_GUARD(outFileGuard, [&zipFilePath]() { base::DeleteFileX(zipFilePath); });

  ZipHandle zip(zipFilePath);
  if (!zip.Handle())
    return false;

  auto const compressionLevel = GetCompressionLevel(compression);
  zip_fileinfo const fileInfo = {};

  try
  {
    for (auto const & filePath : files)
    {
      if (zipOpenNewFileInZip(zip.Handle(), filePath.c_str(), &fileInfo, nullptr, 0, nullptr, 0, "",
                              Z_DEFLATED, compressionLevel) != Z_OK)
      {
        return false;
      }

      base::FileData file(filePath, base::FileData::OP_READ);
      uint64_t const fileSize = file.Size();
      uint64_t writtenSize = 0;
      std::array<char, ZIP_FILE_BUFFER_SIZE> bufferForZip;

      while (writtenSize < fileSize)
      {
        unsigned int const filePartSize =
            std::min(ZIP_FILE_BUFFER_SIZE, static_cast<unsigned int>(fileSize - writtenSize));
        file.Read(writtenSize, bufferForZip.data(), filePartSize);

        if (zipWriteInFileInZip(zip.Handle(), bufferForZip.data(), filePartSize) != ZIP_OK)
          return false;

        writtenSize += filePartSize;
      }
    }
  }
  catch (std::exception const & e)
  {
    LOG(LERROR, ("Error adding files to the archive", zipFilePath, e.what()));
    return false;
  }

  outFileGuard.release();
  return true;
}
