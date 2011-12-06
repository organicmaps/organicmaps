#include "update_generator.hpp"

#include "../defines.hpp"

#include "../platform/platform.hpp"

#include "../storage/country.hpp"

#include "../coding/file_writer.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"
#include "../base/macros.hpp"
#include "../base/timer.hpp"

#include "../std/iterator.hpp"

using namespace storage;

namespace update
{
  // we don't support files without name or without extension
  bool SplitExtension(string const & file, string & name, string & ext)
  {
    // get extension
    size_t const index = file.find_last_of('.');
    if (index == string::npos || (index + 1) == file.size() || index == 0
        || file == "." || file == "..")
    {
      name = file;
      ext.clear();
      return false;
    }
    ext = file.substr(index);
    name = file.substr(0, index);
    return true;
  }

  class SizeUpdater
  {
    size_t m_processedFiles;
    string m_dataDir;
    Platform::FilesList m_allFiles;

  public:
    SizeUpdater(string const & dataDir) : m_processedFiles(0), m_dataDir(dataDir)
    {
      Platform::GetFilesInDir(m_dataDir, "*.mwm", m_allFiles);
    }
    ~SizeUpdater()
    {
      LOG(LINFO, (m_processedFiles, "file sizes were updated in the country list"));
      if (!m_allFiles.empty())
        LOG(LINFO, ("Files left unprocessed:", m_allFiles));
    }
    template <class T>
    void operator()(T & c)
    {
      for (size_t i = 0; i < c.Value().m_files.size(); ++i)
      {
        ++m_processedFiles;
        uint64_t size = 0;
        string const fname = c.Value().m_files[i].GetFileWithExt();
        if (!GetPlatform().GetFileSizeByFullPath(m_dataDir + fname, size))
          LOG(LERROR, ("File was not found:", fname));
        CHECK_GREATER(size, 0, ("Zero file size?", fname));
        c.Value().m_files[i].m_remoteSize = size;
        Platform::FilesList::iterator found = find(m_allFiles.begin(), m_allFiles.end(), fname);
        if (found != m_allFiles.end())
          m_allFiles.erase(found);
      }
    }
  };

  bool UpdateCountries(string const & dataDir)
  {
    Platform::FilesList mwmFiles;
    GetPlatform().GetFilesInDir(dataDir, "*" DATA_FILE_EXTENSION, mwmFiles);

    // remove some files from list
    char const * filesToRemove[] = {"minsk-pass"DATA_FILE_EXTENSION,
                                    "World"DATA_FILE_EXTENSION,
                                    "WorldCoasts"DATA_FILE_EXTENSION};
    for (size_t i = 0; i < ARRAY_SIZE(filesToRemove); ++i)
    {
      Platform::FilesList::iterator found = std::find(mwmFiles.begin(), mwmFiles.end(),
                                                      filesToRemove[i]);
      if (found != mwmFiles.end())
        mwmFiles.erase(found);
    }

    if (mwmFiles.empty())
    {
      LOG(LERROR, ("Can't find any files at path", dataDir));
      return false;
    }
    else
      LOG_SHORT(LINFO, (mwmFiles.size(), "mwm files were found"));

    // load current countries information to update file sizes
    storage::CountriesContainerT countries;
    string jsonBuffer;
    ReaderPtr<Reader>(GetPlatform().GetReader(COUNTRIES_FILE)).ReadAsString(jsonBuffer);
    storage::LoadCountries(jsonBuffer, countries);
    {
      SizeUpdater sizeUpdater(dataDir);
      countries.ForEachChildren(sizeUpdater);
    }

    storage::SaveCountries(my::TodayAsYYMMDD(), countries, jsonBuffer);
    {
      string const outFileName = GetPlatform().WritablePathForFile(COUNTRIES_FILE".updated");
      FileWriter f(outFileName);
      f.Write(&jsonBuffer[0], jsonBuffer.size());
      LOG(LINFO, ("Saved updated countries to", outFileName));
    }

    return true;
  }
} // namespace update
