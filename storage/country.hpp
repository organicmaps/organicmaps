#pragma once

#include "simple_tree.hpp"

#include "../defines.hpp"

#include "../geometry/rect2d.hpp"

#include "../base/buffer_vector.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"


namespace update { class SizeUpdater; }

namespace storage
{
  static int64_t const INVALID_PRICE = -1;

  /// Information about each file for a country
  struct CountryFile
  {
    CountryFile() : m_remoteSize(0), m_price(INVALID_PRICE) {}
    CountryFile(string const & fName, uint32_t remoteSize, int64_t price = -1)
      : m_fileName(fName), m_remoteSize(remoteSize), m_price(price) {}

    string GetFileWithExt() const { return m_fileName + DATA_FILE_EXTENSION; }

    string m_fileName;    /// Same as id of country\region.
    uint32_t m_remoteSize;
    int64_t m_price;
  };
  typedef buffer_vector<CountryFile, 1> FilesContainerT;
  typedef pair<uint64_t, uint64_t> LocalAndRemoteSizeT;

  bool IsFileDownloaded(CountryFile const & file);

  /// Serves as a proxy between GUI and downloaded files
  class Country
  {
    friend class update::SizeUpdater;
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
    int64_t Price() const;

    /// @return bounds for downloaded parts of the country or empty rect
    m2::RectD Bounds() const;
    LocalAndRemoteSizeT Size() const;
  };

  typedef SimpleTree<Country> CountriesContainerT;

  /// @return version of country file or -1 if error was encountered
  int64_t LoadCountries(string const & jsonBuffer, CountriesContainerT & countries);
  void LoadCountryNames(string const & jsonBuffer, map<string, string> & id2name);

  bool SaveCountries(int64_t version, CountriesContainerT const & countries, string & jsonBuffer);
}
