#pragma once
#include "coding/writer.hpp"
#include "base/base.hpp"
#include "std/unique_ptr.hpp"

namespace my { class FileData; }

// FileWriter, not thread safe.
class FileWriter : public Writer
{
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

  /// Works like "move semantics".
  /// Added for use in FilesContainerW interface.
  FileWriter(FileWriter const & rhs);

  explicit FileWriter(string const & fileName,
                      Op operation = OP_WRITE_TRUNCATE, bool bTruncOnClose = false);
  ~FileWriter();

  void Seek(int64_t pos);
  int64_t Pos() const;
  void Write(void const * p, size_t size);

  void WritePaddingByEnd(size_t factor);

  void WritePaddingByPos(size_t factor);

  uint64_t Size() const;
  void Flush();

  void Reserve(uint64_t size);

  static void DeleteFileX(string const & fName);

  string const & GetName() const;

private:
  typedef my::FileData fdata_t;

  void WritePadding(uint64_t offset, uint64_t factor);

  unique_ptr<fdata_t> m_pFileData;
  bool m_bTruncOnClose;
};
