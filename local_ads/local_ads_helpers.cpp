#include "local_ads/local_ads_helpers.hpp"

#include "coding/multilang_utf8_string.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

namespace local_ads
{
void WriteCountryName(FileWriter & writer, std::string const & countryName)
{
  ASSERT(!countryName.empty(), ());
  utils::WriteString(writer, countryName);
}

void WriteDuration(FileWriter & writer, int64_t duration)
{
  uint64_t const encodedDuration = bits::ZigZagEncode(duration);
  WriteToSink(writer, encodedDuration);
}

void WriteRawData(FileWriter & writer, std::vector<uint8_t> const & rawData)
{
  uint64_t const size = static_cast<uint64_t>(rawData.size());
  WriteToSink(writer, size);
  writer.Write(rawData.data(), size);
}

std::string ReadCountryName(ReaderSource<FileReader> & src)
{
  std::string countryName;
  utils::ReadString<decltype(src), true>(src, countryName);
  return countryName;
}

int64_t ReadDuration(ReaderSource<FileReader> & src)
{
  uint64_t const duration = ReadPrimitiveFromSource<uint64_t>(src);
  return bits::ZigZagDecode(duration);
}

std::vector<uint8_t> ReadRawData(ReaderSource<FileReader> & src)
{
  uint64_t const size = ReadPrimitiveFromSource<uint64_t>(src);
  if (src.Size() < size)
    MYTHROW(Reader::SizeException, (src.Pos(), size));
  std::vector<uint8_t> bytes(size);
  src.Read(bytes.data(), bytes.size());
  return bytes;
}
}  // namespace local_ads
