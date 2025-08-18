#pragma once

#include "coding/file_writer.hpp"

#include <cstdint>
#include <string>
#include <vector>

class BufferedFileWriter : public FileWriter
{
public:
  explicit BufferedFileWriter(std::string const & fileName, Op operation = OP_WRITE_TRUNCATE, size_t bufferSize = 4096);

  ~BufferedFileWriter() noexcept(false) override;

  // Writer overrides:
  void Seek(uint64_t pos) override;
  uint64_t Pos() const override;
  void Write(void const * p, size_t size) override;

  // FileWriter overrides:
  uint64_t Size() const override;
  void Flush() override;

private:
  void DropBuffer();

  std::vector<uint8_t> m_buf;
};
