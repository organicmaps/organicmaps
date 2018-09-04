#pragma once

#include "coding/writer.hpp"

#include "base/base.hpp"

#include <memory>

namespace my { class FileData; }

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

  FileWriter(FileWriter && rhs);

  explicit FileWriter(string const & fileName,
                      Op operation = OP_WRITE_TRUNCATE, bool bTruncOnClose = false);
  ~FileWriter() override;

  void Seek(uint64_t pos) override;
  uint64_t Pos() const override;
  void Write(void const * p, size_t size) override;

  void WritePaddingByEnd(size_t factor);
  void WritePaddingByPos(size_t factor);

  uint64_t Size() const;
  void Flush();

  void Reserve(uint64_t size);

  static void DeleteFileX(string const & fName);

  string const & GetName() const;

private:
  void WritePadding(uint64_t offset, uint64_t factor);

  std::unique_ptr<my::FileData> m_pFileData;
  bool m_bTruncOnClose;
};
