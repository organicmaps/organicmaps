#include "coding/zip_creator.hpp"

#include "base/string_utils.hpp"

#include "coding/internal/file_data.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <algorithm>
#include <array>
#include <ctime>
#include <exception>
#include <vector>

namespace
{
class ZipHandle
{
  zip::File m_zipFileHandle;

public:
  explicit ZipHandle(std::string const & filePath)
  {
    m_zipFileHandle = zip::Create(filePath);
  }

  ~ZipHandle()
  {
    if (m_zipFileHandle)
      zip::Close(m_zipFileHandle);
  }

  zip::File Handle() const { return m_zipFileHandle; }
};

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

void FillZipLocalDateTime(zip::DateTime & res)
{
  auto const now = std::time(nullptr);
  // Files inside .zip are using local date time format.
  auto const * local = std::localtime(&now);
  res.tm_sec = local->tm_sec;
  res.tm_min = local->tm_min;
  res.tm_hour = local->tm_hour;
  res.tm_mday = local->tm_mday;
  res.tm_mon = local->tm_mon;
  res.tm_year = local->tm_year;
}

bool CreateZipFromFiles(std::vector<std::string> const & files, std::vector<std::string> const & filesInArchive, std::string const & zipFilePath,
                        CompressionLevel compression)
{
  ASSERT_EQUAL(files.size(), filesInArchive.size(), ("List of file names in archive doesn't match list of files"));
  SCOPE_GUARD(outFileGuard, [&zipFilePath]() { base::DeleteFileX(zipFilePath); });

  ZipHandle zip(zipFilePath);
  if (!zip.Handle())
    return false;

  auto const compressionLevel = GetCompressionLevel(compression);

  try
  {
    int suffix = 0;
    for (size_t i = 0; i < files.size(); i++)
    {
      auto const & filePath = files.at(i);
      std::string fileInArchive = filesInArchive.at(i);
      if (!strings::IsASCIIString(fileInArchive))
      {
        if (suffix == 0)
          fileInArchive = "OrganicMaps.kml";
        else
          fileInArchive = "OrganicMaps_" + std::to_string(suffix) + ".kml";
        ++suffix;
      }
      zip::FileInfo fileInfo = {};
      FillZipLocalDateTime(fileInfo.tmz_date);
      if (zip::Code::Ok != zip::OpenNewFileInZip(zip.Handle(), fileInArchive, fileInfo, "", Z_DEFLATED, compressionLevel))
        return false;

      base::FileData file(filePath, base::FileData::Op::READ);
      uint64_t const fileSize = file.Size();
      uint64_t writtenSize = 0;
      std::array<char, zip::kFileBufferSize> buffer;

      while (writtenSize < fileSize)
      {
        auto const filePartSize = std::min(buffer.size(), static_cast<size_t>(fileSize - writtenSize));
        file.Read(writtenSize, buffer.data(), filePartSize);
        if (zip::Code::Ok != zip::WriteInFileInZip(zip.Handle(), buffer, filePartSize))
          return false;
        writtenSize += filePartSize;
      }
      zipCloseFileInZip(zip.Handle());
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

bool CreateZipFromFiles(std::vector<std::string> const & files, std::string const & zipFilePath,
                        CompressionLevel compression)
{
  std::vector<std::string> filesInArchive;
  for (auto const & file : files)
    filesInArchive.push_back(base::FileNameFromFullPath(file));
  return CreateZipFromFiles(files, filesInArchive, zipFilePath, compression);
}
