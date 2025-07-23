#include "coding/zip_reader.hpp"

#include "coding/constants.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <array>

#include "3party/minizip/minizip.hpp"

namespace
{
class UnzipFileDelegate : public ZipFileReader::Delegate
{
public:
  explicit UnzipFileDelegate(std::string const & path)
    : m_file(std::make_unique<FileWriter>(path))
    , m_path(path)
    , m_completed(false)
  {}

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
  std::unique_ptr<FileWriter> m_file;
  std::string const m_path;
  bool m_completed;
};
}  // namespace

ZipFileReader::ZipFileReader(std::string const & container, std::string const & file, uint32_t logPageSize,
                             uint32_t logPageCount)
  : FileReader(container, logPageSize, logPageCount)
  , m_uncompressedFileSize(0)
{
  auto zip = unzip::Open(container.c_str());
  if (!zip)
    MYTHROW(OpenZipException, ("Can't get zip file handle", container));

  SCOPE_GUARD(zipGuard, std::bind(&unzClose, zip));

  if (unzip::Code::Ok != unzip::GoToFile(zip, file.c_str()))
    MYTHROW(LocateZipException, ("Can't locate file inside zip", file));

  if (unzip::Code::Ok != unzip::OpenCurrentFile(zip))
    MYTHROW(LocateZipException, ("Can't open file inside zip", file));

  auto const offset = unzip::GetCurrentFileFilePos(zip);
  unzip::CloseCurrentFile(zip);

  if (offset == 0 || offset > Size())
    MYTHROW(LocateZipException, ("Invalid offset inside zip", file));

  unzip::FileInfo fileInfo;
  if (unzip::Code::Ok != unzip::GetCurrentFileInfo(zip, fileInfo))
    MYTHROW(LocateZipException, ("Can't get compressed file size inside zip", file));

  SetOffsetAndSize(offset, fileInfo.m_info.compressed_size);
  m_uncompressedFileSize = fileInfo.m_info.uncompressed_size;
}

void ZipFileReader::FilesList(std::string const & zipContainer, FileList & filesList)
{
  auto const zip = unzip::Open(zipContainer.c_str());
  if (!zip)
    MYTHROW(OpenZipException, ("Can't get zip file handle", zipContainer));

  SCOPE_GUARD(zipGuard, std::bind(&unzip::Close, zip));

  if (unzip::Code::Ok != unzip::GoToFirstFile(zip))
    MYTHROW(LocateZipException, ("Can't find first file inside zip", zipContainer));

  do
  {
    unzip::FileInfo fileInfo;
    if (unzip::Code::Ok != unzip::GetCurrentFileInfo(zip, fileInfo))
      MYTHROW(LocateZipException, ("Can't get file name inside zip", zipContainer));

    filesList.push_back(make_pair(fileInfo.m_filename, fileInfo.m_info.uncompressed_size));
  }
  while (unzip::Code::Ok == unzip::GoToNextFile(zip));
}

bool ZipFileReader::IsZip(std::string const & zipContainer)
{
  auto zip = unzip::Open(zipContainer);
  if (!zip)
    return false;
  unzip::Close(zip);
  return true;
}

// static
void ZipFileReader::UnzipFile(std::string const & zipContainer, std::string const & fileInZip, Delegate & delegate)
{
  auto zip = unzip::Open(zipContainer);
  if (!zip)
    MYTHROW(OpenZipException, ("Can't get zip file handle", zipContainer));
  SCOPE_GUARD(zipGuard, std::bind(&unzip::Close, zip));

  if (unzip::Code::Ok != unzip::GoToFile(zip, fileInZip))
    MYTHROW(LocateZipException, ("Can't locate file inside zip", fileInZip));

  if (unzip::Code::Ok != unzip::OpenCurrentFile(zip))
    MYTHROW(LocateZipException, ("Can't open file inside zip", fileInZip));
  SCOPE_GUARD(currentFileGuard, std::bind(&unzip::CloseCurrentFile, zip));

  unzip::FileInfo fileInfo;
  if (unzip::Code::Ok != unzip::GetCurrentFileInfo(zip, fileInfo))
    MYTHROW(LocateZipException, ("Can't get uncompressed file size inside zip", fileInZip));

  std::array<char, unzip::kFileBufferSize> buf;
  int readBytes = 0;

  delegate.OnStarted();
  do
  {
    readBytes = unzip::ReadCurrentFile(zip, buf);
    if (readBytes < 0)
      MYTHROW(InvalidZipException, ("Error", readBytes, "while unzipping", fileInZip, "from", zipContainer));

    delegate.OnBlockUnzipped(static_cast<size_t>(readBytes), buf.data());
  }
  while (readBytes != 0);
  delegate.OnCompleted();
}

// static
void ZipFileReader::UnzipFile(std::string const & zipContainer, std::string const & fileInZip,
                              std::string const & outPath)
{
  UnzipFileDelegate delegate(outPath);
  UnzipFile(zipContainer, fileInZip, delegate);
}
