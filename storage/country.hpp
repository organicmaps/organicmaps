#pragma once

#include "simple_tree.hpp"

#include "../base/buffer_vector.hpp"

#include "../geometry/rect2d.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"

template <class ReaderT> class ReaderPtr;
class Reader;
class Writer;

namespace storage
{
  /// holds file name for tile and it's total size
  typedef pair<string, uint32_t> CountryFile;
  typedef buffer_vector<CountryFile, 1> FilesContainerT;
  typedef pair<uint64_t, uint64_t> LocalAndRemoteSizeT;

  /// Intermediate container for data transfer
  typedef vector<pair<string, uint32_t> > CommonFilesT;

  bool IsFileDownloaded(CountryFile const & file);

  /// Serves as a proxy between GUI and downloaded files
  class Country
  {
    /// Name in the coutry node tree
    string m_name;
    /// Flag to display
    string m_flag;
    /// stores squares with world pieces which are part of the country
    FilesContainerT m_files;

  public:
    Country() {}
    Country(string const & name, string const & flag = "")
      : m_name(name), m_flag(flag) {}

    bool operator<(Country const & other) const { return Name() < other.Name(); }

    void AddFile(CountryFile const & file);
    FilesContainerT const & Files() const { return m_files; }

    string const & Name() const { return m_name; }
    string const & Flag() const { return m_flag; }

    /// @return bounds for downloaded parts of the country or empty rect
    m2::RectD Bounds() const;
    LocalAndRemoteSizeT Size() const;
  };

  typedef SimpleTree<Country> TCountriesContainer;

  /// @param tiles contains files and their sizes
  /// @return false if new application version should be downloaded
  typedef ReaderPtr<Reader> file_t;
  bool LoadCountries(file_t const & file, FilesContainerT const & sortedTiles,
                     TCountriesContainer & countries);
  void SaveFiles(string const & file, CommonFilesT const & commonFiles);
  bool LoadFiles(file_t const & file, FilesContainerT & files, uint32_t & dataVersion);
}
