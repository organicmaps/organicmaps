#pragma once

#include "platform/country_defines.hpp"
#include "std/string.hpp"

namespace platform
{
// This class represents a country file name and sizes of
// corresponding map files on a server, which should correspond to an
// entry in countries.txt file. Also, this class can be used to
// represent a hand-made-country name. Instances of this class don't
// represent paths to disk files.
class CountryFile
{
public:
  CountryFile();
  explicit CountryFile(string const & name);

  string const & GetNameWithoutExt() const;
  string GetNameWithExt(MapOptions file) const;

  void SetRemoteSizes(uint32_t mapSize, uint32_t routingSize);
  uint32_t GetRemoteSize(MapOptions filesMask) const;

  inline bool operator<(const CountryFile & rhs) const { return m_name < rhs.m_name; }
  inline bool operator==(const CountryFile & rhs) const { return m_name == rhs.m_name; }
  inline bool operator!=(const CountryFile & rhs) const { return !(*this == rhs); }

private:
  friend string DebugPrint(CountryFile const & file);

  // Base name (without any extensions) of the file. Same as id of country/region.
  string m_name;
  uint32_t m_mapSize;
  uint32_t m_routingSize;
};

string DebugPrint(CountryFile const & file);
}  // namespace platform
