#pragma once

#include "coding/reader.hpp"

#include "base/base.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

// FileReader, cheap to copy, not thread safe.
// It is assumed that file is not modified during FireReader lifetime,
// because of caching and assumption that Size() is constant.
class FileReader : public ModelReader
{
public:
  static uint32_t const kDefaultLogPageSize;
  static uint32_t const kDefaultLogPageCount;

  explicit FileReader(std::string const & fileName);
  FileReader(std::string const & fileName, uint32_t logPageSize, uint32_t logPageCount);

  // Reader overrides:
  uint64_t Size() const override { return m_size; }
  void Read(uint64_t pos, void * p, size_t size) const override;
  std::unique_ptr<Reader> CreateSubReader(uint64_t pos, uint64_t size) const override;

  FileReader SubReader(uint64_t pos, uint64_t size) const;
  uint64_t GetOffset() const { return m_offset; }

protected:
  // Used in special derived readers.
  void SetOffsetAndSize(uint64_t offset, uint64_t size);

private:
  class FileReaderData;

  FileReader(FileReader const & reader, uint64_t offset, uint64_t size, uint32_t logPageSize, uint32_t logPageCount);

  // Throws an exception if a (pos, size) read would result in an out-of-bounds access.
  void CheckPosAndSize(uint64_t pos, uint64_t size) const;

  uint32_t m_logPageSize;
  uint32_t m_logPageCount;
  std::shared_ptr<FileReaderData> m_fileData;
  uint64_t m_offset;
  uint64_t m_size;
};
