#pragma once

#include "storage/storage_defines.hpp"
#include "storage/simple_tree.hpp"
#include "storage/country_decl.hpp"

#include "platform/country_defines.hpp"

#include "defines.hpp"

#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"


namespace update { class SizeUpdater; }

namespace storage
{
  /// Information about each file for a country
  class CountryFile
  {
    string m_fileName;    /// Same as id of country\region.
    uint32_t m_mapSize, m_routingSize;

  public:
    CountryFile() : m_mapSize(0), m_routingSize(0) {}
    CountryFile(string const & fName, uint32_t mapSize, uint32_t routingSize)
      : m_fileName(fName), m_mapSize(mapSize), m_routingSize(routingSize)
    {
    }

    void AssignSizes(uint32_t mapSize, uint32_t routingSize)
    {
      m_mapSize = mapSize;
      m_routingSize = routingSize;
    }

    string GetFileWithExt(TMapOptions opt) const;
    string const & GetFileWithoutExt() const { return m_fileName; }

    uint32_t GetFileSize(TMapOptions opt) const;
    uint32_t GetRemoteSize(TMapOptions opt) const;
  };

  typedef buffer_vector<CountryFile, 1> FilesContainerT;

  /// Serves as a proxy between GUI and downloaded files
  class Country
  {
    friend class update::SizeUpdater;
    /// Name in the country node tree
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

    size_t GetFilesCount() const { return m_files.size(); }

    /*
    template <class ToDo> void ForEachFile(ToDo toDo) const
    {
      for (FilesContainerT::const_iterator i = m_files.begin(); i != m_files.end(); ++i)
        toDo(i->GetFileWithExt());
    }
    */

    /// This function valid for current logic - one file for one country (region).
    /// If the logic will be changed, replace GetFile with ForEachFile.
    CountryFile const & GetFile() const
    {
      ASSERT_EQUAL ( m_files.size(), 1, (m_name) );
      return m_files.front();
    }

    string const & Name() const { return m_name; }
    string const & Flag() const { return m_flag; }

    /// @return bounds for downloaded parts of the country or empty rect
    //m2::RectD Bounds() const;
    LocalAndRemoteSizeT Size(TMapOptions opt) const;
  };

  typedef SimpleTree<Country> CountriesContainerT;

  /// @return version of country file or -1 if error was encountered
  int64_t LoadCountries(string const & jsonBuffer, CountriesContainerT & countries);
  void LoadCountryFile2CountryInfo(string const & jsonBuffer, map<string, CountryInfo> & id2info);
  void LoadCountryCode2File(string const & jsonBuffer, multimap<string, string> & code2file);

  bool SaveCountries(int64_t version, CountriesContainerT const & countries, string & jsonBuffer);
}
