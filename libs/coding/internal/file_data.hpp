#pragma once

#include "base/macros.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace base
{
class FileData
{
public:
  /// @note Do not change order (@see FileData::FileData).
  enum class Op
  {
    READ = 0,
    WRITE_TRUNCATE,
    WRITE_EXISTING,
    APPEND
  };

  FileData(std::string const & fileName, Op op);
  ~FileData();

  uint64_t Size() const;
  uint64_t Pos() const;

  void Seek(uint64_t pos);

  void Read(uint64_t pos, void * p, size_t size);
  void Write(void const * p, size_t size);

  void Flush();
  void Truncate(uint64_t sz);

  std::string const & GetName() const { return m_FileName; }

private:
  FILE * m_File;
  std::string const m_FileName;
  Op const m_Op;

  std::string GetErrorProlog() const;

  DISALLOW_COPY(FileData);
};

bool GetFileSize(std::string const & fName, uint64_t & sz);
bool DeleteFileX(std::string const & fName);
bool RenameFileX(std::string const & fOld, std::string const & fNew);

/// Write to temp file and rename it to dest. Delete temp on failure.
/// @param write function that writes to file with a given name, returns true on success.
bool WriteToTempAndRenameToFile(std::string const & dest, std::function<bool(std::string const &)> const & write,
                                std::string const & tmp = "");

void AppendFileToFile(std::string const & fromFilename, std::string const & toFilename);

/// @return false if copy fails. DOES NOT THROWS exceptions
bool CopyFileX(std::string const & fOld, std::string const & fNew);
/// @return false if moving fails. DOES NOT THROW exceptions
bool MoveFileX(std::string const & fOld, std::string const & fNew);
bool IsEqualFiles(std::string const & firstFile, std::string const & secondFile);

std::vector<uint8_t> ReadFile(std::string const & filePath);

}  // namespace base
