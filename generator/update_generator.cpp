#include "generator/update_generator.hpp"

#include "defines.hpp"

#include "platform/platform.hpp"

#include "storage/country.hpp"

#include "coding/file_writer.hpp"

#include "base/string_utils.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/timer.hpp"

#include "std/iterator.hpp"

using namespace storage;

namespace update
{
  // we don't support files without name or without extension
  /*
  bool SplitExtension(string const & file, string & name, string & ext)
  {
    // get extension
    size_t const index = file.find_last_of('.');
    if (index == string::npos || (index + 1) == file.size() || index == 0 || file == "." || file == "..")
    {
      name = file;
      ext.clear();
      return false;
    }
    ext = file.substr(index);
    name = file.substr(0, index);
    return true;
  }
  */

  class SizeUpdater
  {
    size_t m_processedFiles;
    string m_dataDir;
    Platform::FilesList & m_files;

    uint64_t GetFileSize(platform::CountryFile const & cnt, MapOptions opt) const
    {
      uint64_t sz = 0;
      string const fName = cnt.GetNameWithExt(opt);
      if (!GetPlatform().GetFileSizeByFullPath(m_dataDir + fName, sz))
      {
        LOG(opt == MapOptions::Map ? LCRITICAL : LWARNING, ("File was not found:", fName));
        return 0;
      }

      CHECK_GREATER(sz, 0, ("Zero file size?", fName));
      return sz;
    }

  public:
    SizeUpdater(string const & dataDir, Platform::FilesList & files)
      : m_processedFiles(0), m_dataDir(dataDir), m_files(files)
    {
    }
    ~SizeUpdater()
    {
      LOG(LINFO, (m_processedFiles, "file sizes were updated in the country list"));

      if (!m_files.empty())
        LOG(LWARNING, ("Files left unprocessed:", m_files));
    }

    template <class T> void operator() (T & c)
    {
      for (size_t i = 0; i < c.Value().m_files.size(); ++i)
      {
        platform::CountryFile & cnt = c.Value().m_files[i];

        ++m_processedFiles;

        uint64_t szMap = GetFileSize(cnt, MapOptions::Map);
        uint64_t szRouting = GetFileSize(cnt, MapOptions::CarRouting);

        ASSERT_EQUAL(static_cast<uint32_t>(szMap), szMap, ());
        ASSERT_EQUAL(static_cast<uint32_t>(szRouting), szRouting, ());

        cnt.SetRemoteSizes(static_cast<uint32_t>(szMap),
                           static_cast<uint32_t>(szRouting));

        string const fName = cnt.GetNameWithExt(MapOptions::Map);
        auto found = find(m_files.begin(), m_files.end(), fName);
        if (found != m_files.end())
          m_files.erase(found);
        else
          LOG(LWARNING, ("No file ", fName, " on disk for the record in countries.txt"));
      }
    }
  };

  bool UpdateCountries(string const & dataDir)
  {
    Platform::FilesList mwmFiles;
    GetPlatform().GetFilesByExt(dataDir, DATA_FILE_EXTENSION, mwmFiles);

    // remove some files from list
    char const * filesToRemove[] = {
            "minsk-pass" DATA_FILE_EXTENSION,
            WORLD_FILE_NAME DATA_FILE_EXTENSION,
            WORLD_COASTS_FILE_NAME DATA_FILE_EXTENSION
    };

    for (size_t i = 0; i < ARRAY_SIZE(filesToRemove); ++i)
    {
      auto found = find(mwmFiles.begin(), mwmFiles.end(), filesToRemove[i]);
      if (found != mwmFiles.end())
        mwmFiles.erase(found);
    }

    if (mwmFiles.empty())
      return false;
    else
      LOG(LINFO, (mwmFiles.size(), "mwm files were found"));

    // load current countries information to update file sizes
    storage::CountriesContainerT countries;
    string jsonBuffer;
    {
      ReaderPtr<Reader>(GetPlatform().GetReader(COUNTRIES_FILE)).ReadAsString(jsonBuffer);
      storage::LoadCountries(jsonBuffer, countries);

      // using move semantics for mwmFiles
      SizeUpdater sizeUpdater(dataDir, mwmFiles);
      countries.ForEachChildren(sizeUpdater);
    }

    storage::SaveCountries(my::TodayAsYYMMDD(), countries, jsonBuffer);
    {
      string const outFileName = dataDir + COUNTRIES_FILE ".updated";
      FileWriter f(outFileName);
      f.Write(&jsonBuffer[0], jsonBuffer.size());
      LOG(LINFO, ("Saved updated countries to", outFileName));
    }

    return true;
  }
} // namespace update
