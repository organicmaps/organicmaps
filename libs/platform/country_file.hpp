#pragma once

#include "platform/country_defines.hpp"

#include <cstdint>
#include <string>

namespace platform
{
/// \param  countryName Country's name without any extensions. For example "Abkhazia".
/// \returns  File name with extension (for download url and save on disk) for \a type. For example "Abkhazia.mwm".
std::string GetFileName(std::string const & countryName, MapFileType type);

/// This class represents a country file name and sizes of
/// corresponding map files on a server, which should correspond to an
/// entry in countries.json file. Also, this class can be used to
/// represent a hand-made-country name. Instances of this class don't
/// represent paths to disk files.
class CountryFile
{
public:
  CountryFile();
  explicit CountryFile(std::string name);
  CountryFile(std::string name, MwmSize size, std::string hash);

  std::string GetFileName(MapFileType type) const { return platform::GetFileName(m_name, type); }

  /// \returns Empty (invalid) CountryFile.
  bool IsEmpty() const { return m_name.empty(); }

  std::string const & GetName() const { return m_name; }
  MwmSize GetRemoteSize() const { return m_mapSize; }
  std::string const & GetHash() const { return m_hash; }

  inline bool operator<(CountryFile const & rhs) const { return m_name < rhs.m_name; }
  inline bool operator==(CountryFile const & rhs) const { return m_name == rhs.m_name; }
  inline bool operator!=(CountryFile const & rhs) const { return !(*this == rhs); }

private:
  friend std::string DebugPrint(CountryFile const & file);

  /// Base name (without any extensions) of the file. Same as id of country/region.
  std::string m_name;
  MwmSize m_mapSize = 0;
  /// \note BLAKE3 integrity hash, truncated and base64-encoded. See coding::Blake3::CalculateMwmBase64.
  std::string m_hash;
};

std::string DebugPrint(CountryFile const & file);
}  // namespace platform
