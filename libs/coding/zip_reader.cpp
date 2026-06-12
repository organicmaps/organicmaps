#include "coding/zip_reader.hpp"

#include "coding/constants.hpp"
#include "coding/zlib.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <array>
#include <cstring>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

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

// ModelReader backed by an owned in-memory buffer (e.g. an inflated zip entry). The buffer is shared,
// so sub-readers keep it alive and stay valid independently of this reader's lifetime.
class MemModelReader : public ModelReader
{
public:
  MemModelReader(std::vector<char> buffer, std::string const & name)
    : ModelReader(name)
    , m_buffer(std::make_shared<std::vector<char>>(std::move(buffer)))
    , m_offset(0)
    , m_size(m_buffer->size())
  {}

  uint64_t Size() const override { return m_size; }

  void Read(uint64_t pos, void * p, size_t size) const override
  {
    CHECK_LESS_OR_EQUAL(pos + size, m_size, (pos, size));
    std::memcpy(p, m_buffer->data() + m_offset + static_cast<size_t>(pos), size);
  }

  // The sub-reader must also be a ModelReader: ModelReaderPtr::SubReader static_casts the result to
  // ModelReader, so returning a plain Reader would be UB.
  std::unique_ptr<Reader> CreateSubReader(uint64_t pos, uint64_t size) const override
  {
    CHECK_LESS_OR_EQUAL(pos + size, m_size, (pos, size));
    // Can't use make_unique with a private constructor.
    return std::unique_ptr<Reader>(
        new MemModelReader(m_buffer, m_offset + static_cast<size_t>(pos), static_cast<size_t>(size), GetName()));
  }

private:
  MemModelReader(std::shared_ptr<std::vector<char>> buffer, size_t offset, size_t size, std::string const & name)
    : ModelReader(name)
    , m_buffer(std::move(buffer))
    , m_offset(offset)
    , m_size(size)
  {}

  std::shared_ptr<std::vector<char>> m_buffer;
  size_t m_offset;
  size_t m_size;
};
}  // namespace

ZipFileReader::ZipFileReader(std::string const & container, std::string const & file, uint32_t logPageSize,
                             uint32_t logPageCount)
  : FileReader(container, logPageSize, logPageCount)
  , m_uncompressedFileSize(0)
  , m_isCompressed(false)
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
  m_isCompressed = fileInfo.m_info.compression_method != 0;
}

// static
std::unique_ptr<ModelReader> ZipFileReader::CreateModelReader(std::string const & zipContainer,
                                                              std::string const & fileInZip, uint32_t logPageSize,
                                                              uint32_t logPageCount)
{
  auto reader = std::make_unique<ZipFileReader>(zipContainer, fileInZip, logPageSize, logPageCount);

  // Stored (uncompressed) entries are read in place from the container without copying — needed for
  // large, memory-mapped/seeked files like .mwm and packed_polygons.bin. ZipFileReader exposes the
  // raw container bytes, so reading in place is correct only for a stored entry (compression method 0);
  // a deflated entry with equal compressed/uncompressed sizes must still be inflated, not read raw.
  if (!reader->IsCompressed())
    return reader;

  // Deflated entry: it can't be read in place, so inflate it once into memory and serve reads there.
  // The reader already maps the entry's raw DEFLATE bytes, so the single open above serves both the
  // header lookup and the data -- inflate straight from them instead of reopening the container.
  uint64_t const uncompressedSize = reader->UncompressedSize();
  std::vector<char> compressed(static_cast<size_t>(reader->Size()));
  reader->Read(0, compressed.data(), compressed.size());
  reader.reset();

  std::vector<char> inflated;
  inflated.reserve(static_cast<size_t>(uncompressedSize));
  coding::ZLib::Inflate const inflate(coding::ZLib::Inflate::Format::Raw);
  if (!inflate(compressed.data(), compressed.size(), std::back_inserter(inflated)))
    MYTHROW(InvalidZipException, ("Can't inflate", fileInZip, "from", zipContainer));

  return std::make_unique<MemModelReader>(std::move(inflated), fileInZip);
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
