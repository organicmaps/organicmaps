#pragma once

#include "platform/country_defines.hpp"

#include <cstdint>
#include <string>

namespace platform
{
/// This class represents a country file name and sizes of
/// corresponding map files on a server, which should correspond to an
/// entry in countries.txt file. Also, this class can be used to
/// represent a hand-made-country name. Instances of this class don't
/// represent paths to disk files.
class CountryFile
{
public:
  CountryFile();
  explicit CountryFile(std::string name);
  CountryFile(std::string name, MwmSize size, std::string sha1);

  /// \returns Empty (invalid) CountryFile.
  bool IsEmpty() const { return m_name.empty(); }

  /// \returns file name without extensions.
  std::string const & GetName() const { return m_name; }
  MwmSize GetRemoteSize() const { return m_mapSize; }
  std::string const & GetSha1() const { return m_sha1; }

  inline bool operator<(const CountryFile & rhs) const { return m_name < rhs.m_name; }
  inline bool operator==(const CountryFile & rhs) const { return m_name == rhs.m_name; }
  inline bool operator!=(const CountryFile & rhs) const { return !(*this == rhs); }

private:
  friend std::string DebugPrint(CountryFile const & file);

  /// Base name (without any extensions) of the file. Same as id of country/region.
  std::string m_name;
  MwmSize m_mapSize = 0;
  /// \note SHA1 is encoded to base64.
  std::string m_sha1;
};

/// \returns This method returns file name with extension. For example Abkhazia.mwm.
/// \param countryFile is a file name without extension. For example Abkhazia.
/// \param type is type of map data.
std::string GetFileName(std::string const & countryFile, MapFileType type);
std::string DebugPrint(CountryFile const & file);
}  // namespace platform
