#include "platform/country_file.hpp"

#include "base/assert.hpp"

#include <sstream>

#include "defines.hpp"

namespace platform
{
std::string GetFileName(std::string const & countryName, MapFileType type)
{
  ASSERT(!countryName.empty(), ());

  switch (type)
  {
  case MapFileType::Map: return countryName + DATA_FILE_EXTENSION;
  case MapFileType::Diff: return countryName + DIFF_FILE_EXTENSION;
  case MapFileType::Count: break;
  }

  UNREACHABLE();
}

CountryFile::CountryFile() : m_mapSize(0) {}

CountryFile::CountryFile(std::string name) : m_name(std::move(name)), m_mapSize(0) {}

CountryFile::CountryFile(std::string name, MwmSize size, std::string sha1)
  : m_name(std::move(name))
  , m_mapSize(size)
  , m_sha1(std::move(sha1))
{}

std::string DebugPrint(CountryFile const & file)
{
  std::ostringstream os;
  os << "CountryFile [" << file.m_name << "]";
  return os.str();
}
}  // namespace platform
