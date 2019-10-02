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
string GetNameWithExt(string const & countryFile, MapOptions file)
{
  switch (file)
  {
    case MapOptions::Map:
      return countryFile + DATA_FILE_EXTENSION;
    case MapOptions::Diff:
      return countryFile + DIFF_FILE_EXTENSION;
    default:
      ASSERT(false, ("Can't get name for:", file));
      return string();
  }
}
}  //  namespace

namespace platform
{
CountryFile::CountryFile() : m_mapSize(0) {}

CountryFile::CountryFile(string const & name) : m_name(name), m_mapSize(0) {}

string const & CountryFile::GetName() const { return m_name; }

void CountryFile::SetRemoteSize(MwmSize mapSize)
{
  m_mapSize = mapSize;
}

MwmSize CountryFile::GetRemoteSize() const
{
  return m_mapSize;
}

string GetFileName(string const & countryFile, MapOptions opt, int64_t version)
{
  if (version::IsSingleMwm(version))
    opt = opt == MapOptions::Diff ? MapOptions::Diff : MapOptions::Map;

  return GetNameWithExt(countryFile, opt);
}

string DebugPrint(CountryFile const & file)
{
  ostringstream os;
  os << "CountryFile [" << file.m_name << "]";
  return os.str();
}
}  // namespace platform
