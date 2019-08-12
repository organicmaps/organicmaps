#pragma once

#include "coding/writer.hpp"

#include "base/base.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace base
{
class FileData;
}

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

  explicit FileWriter(std::string const & fileName,
                      Op operation = OP_WRITE_TRUNCATE);
  FileWriter(FileWriter && rhs) = default;

  ~FileWriter() override;

  // Writer overrides:
  void Seek(uint64_t pos) override;
  uint64_t Pos() const override;
  void Write(void const * p, size_t size) override;

  virtual uint64_t Size() const;
  virtual void Flush();

  std::string const & GetName() const;

  static void DeleteFileX(std::string const & fName);

protected:
  base::FileData & GetFileData();
  base::FileData const & GetFileData() const;

private:
  std::unique_ptr<base::FileData> m_pFileData;
};
