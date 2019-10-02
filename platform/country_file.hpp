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
  explicit CountryFile(std::string const & name);

  /// \returns file name without extensions.
  std::string const & GetName() const;

  /// \note Remote size is size of mwm in bytes. This mwm contains routing and map sections.
  void SetRemoteSize(MwmSize mapSize);
  MwmSize GetRemoteSize() const;

  void SetSha1(std::string const & base64Sha1) { m_sha1 = base64Sha1; }
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

/// \returns This method returns file name with extension. For example Abkhazia.mwm or
/// Abkhazia.mwm.routing.
/// \param countryFile is a file name without extension. For example Abkhazia.
/// \param file is type of map data.
/// \param version is version of mwm. For example 160731.
std::string GetFileName(std::string const & countryFile, MapFileType type, int64_t version);
std::string DebugPrint(CountryFile const & file);
}  // namespace platform
