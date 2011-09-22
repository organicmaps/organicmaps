#pragma once

#include "simple_tree.hpp"

#include "../geometry/rect2d.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"

template <class ReaderT> class ReaderPtr;
class Reader;
class Writer;

namespace storage
{
  /// holds file name for tile and it's total size
  typedef pair<string, uint32_t> TTile;
  typedef vector<TTile> TTilesContainer;
  typedef pair<uint64_t, uint64_t> TLocalAndRemoteSize;

  /// Intermediate container for data transfer
  typedef vector<pair<string, uint32_t> > TCommonFiles;

  bool IsTileDownloaded(TTile const & tile);

  /// Serves as a proxy between GUI and downloaded files
  class Country
  {
    /// Name in the coutry node tree
    string m_name;
    /// Flag to display
    string m_flag;
    /// stores squares with world pieces which are part of the country
    TTilesContainer m_tiles;

  public:
    Country() {}
    Country(string const & name, string const & flag = "")
      : m_name(name), m_flag(flag) {}

    bool operator<(Country const & other) const { return Name() < other.Name(); }

    void AddTile(TTile const & tile);
    TTilesContainer const & Tiles() const { return m_tiles; }

    string const & Name() const { return m_name; }
    string const & Flag() const { return m_flag; }

    /// @return bounds for downloaded parts of the country or empty rect
    m2::RectD Bounds() const;
    TLocalAndRemoteSize Size() const;
  };

  typedef SimpleTree<Country> TCountriesContainer;

  /// @param tiles contains files and their sizes
  /// @return false if new application version should be downloaded
  typedef ReaderPtr<Reader> file_t;
  bool LoadCountries(file_t const & file, TTilesContainer const & sortedTiles,
                     TCountriesContainer & countries);
  void SaveTiles(string const & file, TCommonFiles const & commonFiles);
  bool LoadTiles(file_t const & file, TTilesContainer & tiles, uint32_t & dataVersion);
  //void SaveCountries(TCountriesContainer const & countries, Writer & writer);
}
