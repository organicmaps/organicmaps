#pragma once
#include "file64_api.hpp"

#include "../../base/base.hpp"

#include "../../std/string.hpp"

#ifdef OMIM_OS_BADA
  #include <FIoFile.h>
#endif


namespace my {

class FileData
{
public:
  enum Op { OP_READ = 0, OP_WRITE_TRUNCATE = 1, OP_WRITE_EXISTING = 2, OP_APPEND};

  FileData(string const & fileName, Op op);
  ~FileData();

  uint64_t Size() const;
  uint64_t Pos() const;

  void Seek(uint64_t pos);

  void Read(uint64_t pos, void * p, size_t size);
  void Write(void const * p, size_t size);

  void Flush();
  void Truncate(uint64_t sz);

  string GetName() const { return m_FileName; }

private:
  uint64_t m_LastAccessPos;
#ifdef OMIM_OS_BADA
  Osp::Io::File m_File;
#else
  FILE * m_File;
#endif
  string m_FileName;
  Op m_Op;
};

bool GetFileSize(string const & fName, uint64_t & sz);
bool DeleteFileX(string const & fName);
bool RenameFileX(string const & fOld, string const & fNew);
/// @return false if copy fails. DO NOT THROWS exceptions
bool CopyFile(string const & fOld, string const & fNew);
bool IsEqualFiles(string const & firstFile, string const & secondFile);

}
