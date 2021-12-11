#include "platform/country_file.hpp"

#include "platform/mwm_version.hpp"

#include "base/assert.hpp"

#include <sstream>

#include "defines.hpp"

using namespace std;

namespace
{
/// \returns file name (m_name) with extension dependent on the file param.
/// The extension could be .mwm.routing or just .mwm.
/// The method is used for old (two components) mwm support.
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

string GetFileName(string const & countryFile, MapFileType type)
{
  return GetNameWithExt(countryFile, type);
}

string DebugPrint(CountryFile const & file)
{
  ostringstream os;
  os << "CountryFile [" << file.m_name << "]";
  return os.str();
}
}  // namespace platform
