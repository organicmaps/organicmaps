#include "update_generator.hpp"

#include "../../coding/file_writer.hpp"

#include "../../platform/platform.hpp"

#include "../../indexer/country.hpp"
#include "../../indexer/defines.hpp"

#include "../../base/string_utils.hpp"
#include "../../base/logging.hpp"

#include "../../std/target_os.hpp"
#include "../../std/fstream.hpp"

#define GROUP_FILE_EXTENSION ".group"
#define REGION_FILE_EXTENSION ".regions"
#define CELLID_FILE_EXTENSION ".cells"

#define PREFIX_CHAR '-'

#ifdef OMIM_OS_WINDOWS_NATIVE
  #define DIR_SEP '\\'
#else
  #define DIR_SEP '/'
#endif

namespace update
{
  typedef vector<string> TCellIds;
  typedef pair<mapinfo::Country, TCellIds> TCountryCells;
  typedef vector<TCountryCells> TCountryCellsContainer;

  string ChopExtension(string const & nameWithExtension)
  {
    size_t dotPos = nameWithExtension.rfind('.');
    return nameWithExtension.substr(0, dotPos);
  }

  string ChopPrefix(string const & nameWithPrefix)
  {
    size_t prefixPos = nameWithPrefix.rfind(PREFIX_CHAR);
    if (prefixPos != string::npos && (prefixPos + 1) < nameWithPrefix.size())
      return nameWithPrefix.substr(prefixPos + 1, string::npos);
    else
      return nameWithPrefix;
  }

  bool LoadCountryCells(string const & fullPath, TCellIds & outCells)
  {
    outCells.clear();
    ifstream file(fullPath.c_str());
    string cell;
    while (file.good())
    {
      getline(file, cell);
      if (cell.size())
        outCells.push_back(cell);
    }
    return !outCells.empty();
  }

  /// @param path where folders with groups reside (Africa, Asia etc.)
  /// @return true if loaded correctly
  bool ScanAndLoadCountryCells(string path, TCountryCellsContainer & outCells)
  {
    if (path.empty())
      return false;
    // fix missing slash
    if (path[path.size() - 1] != DIR_SEP)
      path.push_back(DIR_SEP);

    outCells.clear();

    Platform & platform = GetPlatform();
    // get all groups
    Platform::FilesList groups;
    platform.GetFilesInDir(path, "*" GROUP_FILE_EXTENSION, groups);
    for (Platform::FilesList::iterator itGroup = groups.begin(); itGroup != groups.end(); ++itGroup)
    {
      // get countries with regions
      Platform::FilesList countries;
      platform.GetFilesInDir(path + DIR_SEP + *itGroup, "*" REGION_FILE_EXTENSION, countries);
      for (Platform::FilesList::iterator itCountry = countries.begin(); itCountry != countries.end(); ++itCountry)
      { // get all regions
        Platform::FilesList regions;
        platform.GetFilesInDir(path + DIR_SEP + *itGroup + DIR_SEP + *itCountry,
                               "*" CELLID_FILE_EXTENSION, regions);
        for (Platform::FilesList::iterator itRegion = regions.begin(); itRegion != regions.end(); ++itRegion)
        {
          TCellIds cells;
          string fullPath = path + *itGroup + DIR_SEP + *itCountry + DIR_SEP + *itRegion;
          if (LoadCountryCells(fullPath, cells))
          {
            outCells.push_back(TCountryCells(mapinfo::Country(ChopExtension(*itGroup),
                                                       ChopExtension(*itCountry),
                                                       ChopExtension(*itRegion)),
                                      cells));
          }
          else
          {
            LOG(LERROR, ("Can't load cells from file", fullPath));
          }
        }
      }

      // get countries without regions
      countries.clear();
      platform.GetFilesInDir(path + DIR_SEP + *itGroup, "*" CELLID_FILE_EXTENSION, countries);
      for (Platform::FilesList::iterator itCountry = countries.begin(); itCountry != countries.end(); ++itCountry)
      {
        TCellIds cells;
        string fullPath = path + *itGroup + DIR_SEP + *itCountry;
        if (LoadCountryCells(fullPath, cells))
        {
          outCells.push_back(TCountryCells(mapinfo::Country(ChopExtension(*itGroup),
                                                     ChopExtension(*itCountry),
                                                     ""),
                                    cells));
        }
        else
        {
          LOG(LERROR, ("Can't load cells from file", fullPath));
        }
      }
    }
    return !outCells.empty();
  }

  class CellChecker
  {
    string const m_path, m_file, m_url;

  public:
    CellChecker(string const & dataPath, string const & dataFileName, string const & baseUrl)
      : m_path(dataPath), m_file(dataFileName), m_url(baseUrl) {}
    void operator()(TCountryCells & cells) const
    {
      string const fileCell = ChopPrefix(ChopExtension(m_file));
      for (TCellIds::iterator it = cells.second.begin(); it != cells.second.end(); ++it)
      {
        // check if country contains tile with this cell
        if (fileCell.find(*it) == 0 || it->find(fileCell) == 0)
        {
          // data file
          uint64_t fileSize = 0;
          CHECK(GetPlatform().GetFileSize(m_path + m_file, fileSize), ("Non-existing file?"));
          cells.first.AddUrl(mapinfo::TUrl(m_url + m_file, fileSize));
          // index file
          string const indexFileName = mapinfo::IndexFileForDatFile(m_file);
          CHECK(GetPlatform().GetFileSize(m_path + indexFileName, fileSize), ("Non-existing file?"));
          cells.first.AddUrl(mapinfo::TUrl(m_url + indexFileName, fileSize));
          break;
        }
      }
    }
  };

  class CountryAdder
  {
    mapinfo::TCountriesContainer & m_countries;

  public:
    CountryAdder(mapinfo::TCountriesContainer & outCountries)
      : m_countries(outCountries) {}
    void operator()(TCountryCells const & cells)
    {
      if (cells.first.Urls().size())
        m_countries[cells.first.Group()].push_back(cells.first);
    }
  };

  class GroupSorter
  {
  public:
    void operator()(mapinfo::TCountriesContainer::value_type & toSort)
    {
      sort(toSort.second.begin(), toSort.second.end());
    }
  };

  bool GenerateMapsList(string const & pathToMaps, string const & pathToCountries, string const & baseUrl)
  {
    Platform & platform = GetPlatform();

    TCountryCellsContainer countriesCells;
    if (!ScanAndLoadCountryCells(pathToCountries, countriesCells))
    {
      LOG(LERROR, ("Can't load countries' cells from path", pathToCountries));
      return false;
    }

    Platform::FilesList datFiles;
    if (!platform.GetFilesInDir(pathToMaps, "*" DATA_FILE_EXTENSION, datFiles))
    {
      LOG(LERROR, ("Can't find any data files at path", pathToMaps));
      return false;
    }

    // update each country's urls corresponding to existing data files
    for (Platform::FilesList::iterator it = datFiles.begin(); it != datFiles.end(); ++it)
    {
      for_each(countriesCells.begin(), countriesCells.end(), CellChecker(pathToMaps, *it, baseUrl));
    }

    // save update list
    mapinfo::TCountriesContainer countries;
    for_each(countriesCells.begin(), countriesCells.end(), CountryAdder(countries));

    // sort groups
    for_each(countries.begin(), countries.end(), GroupSorter());

    FileWriter writer(pathToMaps + UPDATE_CHECK_FILE);
    SaveCountries(countries, writer);
    return true;
  }
} // namespace update
