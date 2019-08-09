#pragma once

#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include <cstdint>
#include <string>
#include <vector>

class ContainerFileWriter : public FileWriter
{
public:
  ContainerFileWriter(std::string const & fileName, Op operation)
    : FileWriter(fileName, operation)
  {
  }

  void WritePaddingByEnd(size_t factor) { WritePadding(Size(), factor); }
  void WritePaddingByPos(size_t factor) { WritePadding(Pos(), factor); }

private:
  void WritePadding(uint64_t offset, uint64_t factor)
  {
    ASSERT(factor > 1, ());
    uint64_t const padding = ((offset + factor - 1) / factor) * factor - offset;
    if (!padding)
      return;
    std::vector<uint8_t> buffer(static_cast<size_t>(padding));
    Write(buffer.data(), buffer.size());
  }
};

class TruncatingFileWriter : public ContainerFileWriter
{
public:
  explicit TruncatingFileWriter(std::string const & fileName)
    : ContainerFileWriter(fileName, FileWriter::OP_WRITE_EXISTING)
  {
  }

  TruncatingFileWriter(TruncatingFileWriter && rhs) = default;

  ~TruncatingFileWriter() override
  {
    GetFileData().Flush();
    GetFileData().Truncate(Pos());
  }
};
