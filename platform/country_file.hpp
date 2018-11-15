#pragma once

#include "platform/country_defines.hpp"
#include "std/string.hpp"

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
  explicit CountryFile(string const & name);

  /// \returns file name without extensions.
  string const & GetName() const;

  /// \note Remote size is size of mwm in bytes. This mwm contains routing and map sections.
  void SetRemoteSizes(TMwmSize mapSize, TMwmSize routingSize);
  TMwmSize GetRemoteSize(MapOptions file) const;

  void SetSha1(string const & base64Sha1) { m_sha1 = base64Sha1; }
  string const & GetSha1() const { return m_sha1; }

  inline bool operator<(const CountryFile & rhs) const { return m_name < rhs.m_name; }
  inline bool operator==(const CountryFile & rhs) const { return m_name == rhs.m_name; }
  inline bool operator!=(const CountryFile & rhs) const { return !(*this == rhs); }

private:
  friend string DebugPrint(CountryFile const & file);

  /// Base name (without any extensions) of the file. Same as id of country/region.
  string m_name;
  TMwmSize m_mapSize = 0;
  TMwmSize m_routingSize = 0;
  /// \note SHA1 is encoded to base64.
  string m_sha1;
};

/// \returns This method returns file name with extension. For example Abkhazia.mwm or
/// Abkhazia.mwm.routing.
/// \param countryFile is a file name without extension. For example Abkhazia.
/// \param file is type of map data.
/// \param version is version of mwm. For example 160731.
string GetFileName(string const & countryFile, MapOptions file, int64_t version);
string DebugPrint(CountryFile const & file);
}  // namespace platform
