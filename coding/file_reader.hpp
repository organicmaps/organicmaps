#pragma once
#include "reader.hpp"
#include "../base/base.hpp"
#include "../std/shared_ptr.hpp"

// FileReader, cheap to copy, not thread safe.
// It is assumed that file is not modified during FireReader lifetime,
// because of caching and assumption that Size() is constant.
class FileReader : public Reader
{
public:
  explicit FileReader(string const & fileName,
                      uint32_t logPageSize = 10,
                      uint32_t logPageCount = 8);

  class FileReaderData;

  uint64_t Size() const;
  void Read(uint64_t pos, void * p, size_t size) const;
  FileReader SubReader(uint64_t pos, uint64_t size) const;
  FileReader * CreateSubReader(uint64_t pos, uint64_t size) const;

  bool IsEqual(string const & fName) const;
  string GetName() const;
  string ReadAsText() const;

private:
  FileReader(shared_ptr<FileReaderData> const & pFileData, uint64_t offset, uint64_t size);

  shared_ptr<FileReaderData> m_pFileData;
  uint64_t m_Offset;
  uint64_t m_Size;
};
