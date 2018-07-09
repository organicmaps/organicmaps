#include "coding/zip_reader.hpp"

#include "base/scope_guard.hpp"
#include "base/logging.hpp"

#include "coding/constants.hpp"

#include "std/bind.hpp"

#include "3party/minizip/unzip.h"

namespace
{
class UnzipFileDelegate : public ZipFileReader::Delegate
{
public:
  UnzipFileDelegate(string const & path)
    : m_file(make_unique<FileWriter>(path)), m_path(path), m_completed(false)
  {
  }

  ~UnzipFileDelegate() override
  {
    if (!m_completed)
    {
      m_file.reset();
      FileWriter::DeleteFileX(m_path);
    }
  }

  // ZipFileReader::Delegate overrides:
  void OnBlockUnzipped(size_t size, char const * data) override { m_file->Write(data, size); }

  void OnCompleted() override { m_completed = true; }

private:
  unique_ptr<FileWriter> m_file;
  string const m_path;
  bool m_completed;
};
}  // namespace

ZipFileReader::ZipFileReader(string const & container, string const & file, uint32_t logPageSize,
                             uint32_t logPageCount)
  : FileReader(container, logPageSize, logPageCount), m_uncompressedFileSize(0)
{
  unzFile zip = unzOpen64(container.c_str());
  if (!zip)
    MYTHROW(OpenZipException, ("Can't get zip file handle", container));

  MY_SCOPE_GUARD(zipGuard, bind(&unzClose, zip));

  if (UNZ_OK != unzLocateFile(zip, file.c_str(), 1))
    MYTHROW(LocateZipException, ("Can't locate file inside zip", file));

  if (UNZ_OK != unzOpenCurrentFile(zip))
    MYTHROW(LocateZipException, ("Can't open file inside zip", file));

  uint64_t const offset = unzGetCurrentFileZStreamPos64(zip);
  (void) unzCloseCurrentFile(zip);

  if (offset == 0 || offset > Size())
    MYTHROW(LocateZipException, ("Invalid offset inside zip", file));

  unz_file_info64 fileInfo;
  if (UNZ_OK != unzGetCurrentFileInfo64(zip, &fileInfo, NULL, 0, NULL, 0, NULL, 0))
    MYTHROW(LocateZipException, ("Can't get compressed file size inside zip", file));

  SetOffsetAndSize(offset, fileInfo.compressed_size);
  m_uncompressedFileSize = fileInfo.uncompressed_size;
}

void ZipFileReader::FilesList(string const & zipContainer, FileListT & filesList)
{
  unzFile const zip = unzOpen64(zipContainer.c_str());
  if (!zip)
    MYTHROW(OpenZipException, ("Can't get zip file handle", zipContainer));

  MY_SCOPE_GUARD(zipGuard, bind(&unzClose, zip));

  if (UNZ_OK != unzGoToFirstFile(zip))
    MYTHROW(LocateZipException, ("Can't find first file inside zip", zipContainer));

  do
  {
    char fileName[256];
    unz_file_info64 fileInfo;
    if (UNZ_OK != unzGetCurrentFileInfo64(zip, &fileInfo, fileName, ARRAY_SIZE(fileName), NULL, 0, NULL, 0))
      MYTHROW(LocateZipException, ("Can't get file name inside zip", zipContainer));

    filesList.push_back(make_pair(fileName, fileInfo.uncompressed_size));

  } while (UNZ_OK == unzGoToNextFile(zip));
}

bool ZipFileReader::IsZip(string const & zipContainer)
{
  unzFile zip = unzOpen64(zipContainer.c_str());
  if (!zip)
    return false;
  unzClose(zip);
  return true;
}

// static
void ZipFileReader::UnzipFile(string const & zipContainer, string const & fileInZip,
                              Delegate & delegate)
{
  unzFile zip = unzOpen64(zipContainer.c_str());
  if (!zip)
    MYTHROW(OpenZipException, ("Can't get zip file handle", zipContainer));
  MY_SCOPE_GUARD(zipGuard, bind(&unzClose, zip));

  if (UNZ_OK != unzLocateFile(zip, fileInZip.c_str(), 1))
    MYTHROW(LocateZipException, ("Can't locate file inside zip", fileInZip));

  if (UNZ_OK != unzOpenCurrentFile(zip))
    MYTHROW(LocateZipException, ("Can't open file inside zip", fileInZip));
  MY_SCOPE_GUARD(currentFileGuard, bind(&unzCloseCurrentFile, zip));

  unz_file_info64 fileInfo;
  if (UNZ_OK != unzGetCurrentFileInfo64(zip, &fileInfo, NULL, 0, NULL, 0, NULL, 0))
    MYTHROW(LocateZipException, ("Can't get uncompressed file size inside zip", fileInZip));

  char buf[ZIP_FILE_BUFFER_SIZE];
  int readBytes = 0;

  delegate.OnStarted();
  do
  {
    readBytes = unzReadCurrentFile(zip, buf, ZIP_FILE_BUFFER_SIZE);
    if (readBytes < 0)
    {
      MYTHROW(InvalidZipException,
              ("Error", readBytes, "while unzipping", fileInZip, "from", zipContainer));
    }

    delegate.OnBlockUnzipped(static_cast<size_t>(readBytes), buf);
  } while (readBytes != 0);
  delegate.OnCompleted();
}

// static
void ZipFileReader::UnzipFile(string const & zipContainer, string const & fileInZip,
                              string const & outPath)
{
  UnzipFileDelegate delegate(outPath);
  UnzipFile(zipContainer, fileInZip, delegate);
}
