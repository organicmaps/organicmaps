#include "platform/country_file.hpp"

#include "defines.hpp"
#include "base/assert.hpp"
#include "std/sstream.hpp"

namespace platform
{
CountryFile::CountryFile() : m_mapSize(0), m_routingSize(0) {}

CountryFile::CountryFile(string const & name) : m_name(name), m_mapSize(0), m_routingSize(0) {}

string const & CountryFile::GetNameWithoutExt() const { return m_name; }

string CountryFile::GetNameWithExt(TMapOptions file) const
{
  switch (file)
  {
    case TMapOptions::Map:
      return m_name + DATA_FILE_EXTENSION;
    case TMapOptions::CarRouting:
      return m_name + DATA_FILE_EXTENSION + ROUTING_FILE_EXTENSION;
    default:
      ASSERT(false, ("Can't get name for:", file));
      return string();
  }
}

void CountryFile::SetRemoteSizes(uint32_t mapSize, uint32_t routingSize)
{
  m_mapSize = mapSize;
  m_routingSize = routingSize;
}

uint32_t CountryFile::GetRemoteSize(TMapOptions filesMask) const
{
  uint32_t size = 0;
  if (HasOptions(filesMask, TMapOptions::Map))
    size += m_mapSize;
  if (HasOptions(filesMask, TMapOptions::CarRouting))
    size += m_routingSize;
  return size;
}

string DebugPrint(CountryFile const & file)
{
  ostringstream os;
  os << "CountryFile [" << file.m_name << "]";
  return os.str();
}
}  // namespace platform
