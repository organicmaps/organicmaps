#include "coding/reader.hpp"
#include "coding/memory_region.hpp"

std::unique_ptr<MemoryRegion> Reader::GetMemoryRegion(uint64_t pos, size_t size) const
{
  std::vector<uint8_t> buffer(size);
  Read(pos, buffer.data(), size);
  return std::make_unique<CopiedMemoryRegion>(std::move(buffer));
}

void Reader::ReadAsString(std::string & s) const
{
  s.clear();
  s.resize(static_cast<size_t>(Size()));
  Read(0 /* pos */, s.data(), s.size());
}
