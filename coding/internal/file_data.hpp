#pragma once
#include "coding/internal/file64_api.hpp"

#include "base/base.hpp"

#include "std/functional.hpp"
#include "std/noncopyable.hpp"
#include "std/string.hpp"
#include "std/target_os.hpp"

#ifdef OMIM_OS_TIZEN
namespace Tizen
{
  namespace Io
  {
    class File;
  }
}
#endif


namespace my {

class FileData: private noncopyable
{
public:
  /// @note Do not change order (@see FileData::FileData).
  enum Op { OP_READ = 0, OP_WRITE_TRUNCATE, OP_WRITE_EXISTING, OP_APPEND };

  FileData(string const & fileName, Op op);
  ~FileData();

  uint64_t Size() const;
  uint64_t Pos() const;

  void Seek(uint64_t pos);

  void Read(uint64_t pos, void * p, size_t size);
  void Write(void const * p, size_t size);

  void Flush();
  void Truncate(uint64_t sz);

  string const & GetName() const { return m_FileName; }

private:

#ifdef OMIM_OS_TIZEN
  Tizen::Io::File * m_File;
#else
  FILE * m_File;
#endif
  string m_FileName;
  Op m_Op;

  string GetErrorProlog() const;
};

bool GetFileSize(string const & fName, uint64_t & sz);
bool DeleteFileX(string const & fName);
bool RenameFileX(string const & fOld, string const & fNew);

/// Write to temp file and rename it to dest. Delete temp on failure.
/// @param write function that writes to file with a given name, returns true on success.
bool WriteToTempAndRenameToFile(string const & dest, function<bool(string const &)> const & write,
                                string const & tmp = "");

/// @return false if copy fails. DO NOT THROWS exceptions
bool CopyFileX(string const & fOld, string const & fNew);
bool IsEqualFiles(string const & firstFile, string const & secondFile);
}
