#pragma once
#include "coding/reader.hpp"
#include "base/base.hpp"
#include "std/shared_ptr.hpp"

// FileReader, cheap to copy, not thread safe.
// It is assumed that file is not modified during FireReader lifetime,
// because of caching and assumption that Size() is constant.
class FileReader : public ModelReader
{
  using BaseType = ModelReader;

public:
  explicit FileReader(string const & fileName,
                      bool withExceptions = false,
                      uint32_t logPageSize = 10,
                      uint32_t logPageCount = 4);

  class FileReaderData;

  uint64_t Size() const override;
  void Read(uint64_t pos, void * p, size_t size) const override;
  FileReader SubReader(uint64_t pos, uint64_t size) const;
  unique_ptr<Reader> CreateSubReader(uint64_t pos, uint64_t size) const override;

  inline uint64_t GetOffset() const { return m_offset; }

protected:
  void CheckPosAndSize(uint64_t pos, uint64_t size) const;

  /// Make assertion that pos + size in FileReader bounds.
  bool AssertPosAndSize(uint64_t pos, uint64_t size) const;
  /// Used in special derived readers.
  void SetOffsetAndSize(uint64_t offset, uint64_t size);

private:
  FileReader(FileReader const & reader, uint64_t offset, uint64_t size);

  shared_ptr<FileReaderData> m_fileData;
  uint64_t m_offset;
  uint64_t m_size;
  bool m_withExceptions;
};
