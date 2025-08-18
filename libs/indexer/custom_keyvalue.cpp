#include "custom_keyvalue.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include <sstream>

namespace indexer
{

CustomKeyValue::CustomKeyValue(std::string_view buffer)
{
  MemReader reader(buffer.data(), buffer.size());
  ReaderSource src(reader);

  while (src.Size() > 0)
  {
    uint8_t const type = ReadPrimitiveFromSource<uint8_t>(src);
    m_vals.emplace_back(type, ReadVarUint<uint64_t>(src));
  }
}

std::string CustomKeyValue::ToString() const
{
  std::string res;
  MemWriter<std::string> writer(res);
  for (auto const & v : m_vals)
  {
    WriteToSink(writer, v.first);
    WriteVarUint(writer, v.second);
  }
  return res;
}

std::string DebugPrint(CustomKeyValue const & kv)
{
  std::ostringstream ss;
  ss << "CustomKeyValue [";
  for (auto const & v : kv.m_vals)
    ss << "(" << v.first << ", " << v.second << "), ";
  ss << "]";
  return ss.str();
}

}  // namespace indexer
