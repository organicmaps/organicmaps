#pragma once

#include "coding/internal/file_data.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

// FileWriter, not thread safe.
class FileWriter : public Writer
{
  DISALLOW_COPY(FileWriter);

public:
  // Values actually match internal FileData::Op enum.
  enum Op
  {
    // Create an empty file for writing. If a file with the same name already exists
    // its content is erased and the file is treated as a new empty file.
    OP_WRITE_TRUNCATE = 1,

    // Open a file for update. The file is created if it does not exist.
    OP_WRITE_EXISTING = 2,

    // Append to a file. Writing operations append data at the end of the file.
    // The file is created if it does not exist.
    // Seek should not be called, if file is opened for append.
    OP_APPEND = 3
  };

  explicit FileWriter(std::string const & fileName, Op operation = OP_WRITE_TRUNCATE);
  FileWriter(FileWriter && rhs) = default;

  virtual ~FileWriter() noexcept(false);

  // Writer overrides:
  void Seek(uint64_t pos) override;
  uint64_t Pos() const override;
  void Write(void const * p, size_t size) override;

  virtual uint64_t Size() const;
  virtual void Flush() noexcept(false);

  std::string const & GetName() const;

  static void DeleteFileX(std::string const & fName);

protected:
  std::unique_ptr<base::FileData> m_pFileData;
};

class FilesContainerWriter : public FileWriter
{
public:
  FilesContainerWriter(std::string const & fileName, Op operation) : FileWriter(fileName, operation) {}

  void WritePaddingByEnd(size_t factor) { WritePadding(Size(), factor); }
  void WritePaddingByPos(size_t factor) { WritePadding(Pos(), factor); }

private:
  void WritePadding(uint64_t offset, uint64_t factor)
  {
    ASSERT_GREATER(factor, 1, ());
    uint64_t const padding = ((offset + factor - 1) / factor) * factor - offset;
    if (padding == 0)
      return;
    WriteZeroesToSink(*this, padding);
  }
};

class TruncatingFileWriter : public FilesContainerWriter
{
public:
  explicit TruncatingFileWriter(std::string const & fileName)
    : FilesContainerWriter(fileName, FileWriter::OP_WRITE_EXISTING)
  {}

  TruncatingFileWriter(TruncatingFileWriter && rhs) = default;

  ~TruncatingFileWriter() noexcept(false) override
  {
    m_pFileData->Flush();
    m_pFileData->Truncate(Pos());
  }
};
