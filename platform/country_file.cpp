#include "platform/country_file.hpp"

#include "platform/mwm_version.hpp"

#include "base/assert.hpp"

#include <sstream>

#include "defines.hpp"

using namespace std;

namespace
{
/// \returns Map's file name with extension depending on the \a file type.
string GetNameWithExt(string const & countryFile, MapFileType file)
{
  switch (file)
  {
  case MapFileType::Map: return countryFile + DATA_FILE_EXTENSION;
  case MapFileType::Diff: return countryFile + DIFF_FILE_EXTENSION;
  case MapFileType::Count: CHECK(false, (countryFile));
  }

  UNREACHABLE();
}
}  //  namespace

namespace platform
{
CountryFile::CountryFile() : m_mapSize(0) {}

CountryFile::CountryFile(std::string name)
: m_name(std::move(name)), m_mapSize(0)
{
}

CountryFile::CountryFile(std::string name, MwmSize size, std::string sha1)
: m_name(std::move(name)), m_mapSize(size), m_sha1(sha1)
{
}

string CountryFile::GetFileName(MapFileType type) const
{
  ASSERT(!m_name.empty(), ());

  if (m_name == RESOURCES_FILE_NAME)
  {
    std::string res = m_name + RESOURCES_EXTENSION;
    // Map and Diff should be different to avoid conflicts.
    if (type == MapFileType::Diff)
      res += "diff";
    return res;
  }

  return GetNameWithExt(m_name, type);
}

string DebugPrint(CountryFile const & file)
{
  ostringstream os;
  os << "CountryFile [" << file.m_name << "]";
  return os.str();
}
}  // namespace platform
