#include "update_generator.hpp"

#include "../../coding/file_writer.hpp"

#include "../../geometry/cellid.hpp"

#include "../../platform/platform.hpp"

#include "../../storage/country.hpp"
#include "../../defines.hpp"

#include "../../base/string_utils.hpp"
#include "../../base/logging.hpp"
#include "../../base/macros.hpp"

#include "../../std/target_os.hpp"
#include "../../std/fstream.hpp"
#include "../../std/iterator.hpp"

using namespace storage;

/// files which can be updated through downloader
char const * gExtensionsToUpdate[] = {
  "*" DATA_FILE_EXTENSION, "*.txt", "*.bin", "*.skn", "*.ttf", "*.png"
};

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

  bool GenerateFilesList(string const & dataDir)
  {
    Platform & platform = GetPlatform();

    Platform::FilesList files;
    for (size_t i = 0; i < ARRAY_SIZE(gExtensionsToUpdate); ++i)
    {
      Platform::FilesList otherFiles;
      platform.GetFilesInDir(dataDir, gExtensionsToUpdate[i], otherFiles);
      std::copy(otherFiles.begin(), otherFiles.end(), std::back_inserter(files));
    }
    { // remove minsk-pass from list
      Platform::FilesList::iterator minskPassIt = std::find(files.begin(), files.end(), "minsk-pass" DATA_FILE_EXTENSION);
      if (minskPassIt != files.end())
        files.erase(minskPassIt);
    }
    if (files.empty())
    {
      LOG(LERROR, ("Can't find any files at path", dataDir));
      return false;
    }
    else
    {
      LOG_SHORT(LINFO, ("Files count included in update file:", files.size()));
    }

    TDataFiles cellFiles;
    TCommonFiles commonFiles;
    string name, ext;
    int32_t level = -1;
    uint16_t bits;
    for (Platform::FilesList::iterator it = files.begin(); it != files.end(); ++it)
    {
      uint64_t size = 0;
      CHECK( platform.GetFileSize(dataDir + *it, size), ());
      CHECK_EQUAL( size, static_cast<uint32_t>(size), ("We don't support files > 4gb", *it));
      if (SplitExtension(*it, name, ext))
      {
        // is it data cell file?
        if (ext == DATA_FILE_EXTENSION)
        {
          if (CountryCellId::IsCellId(name))
          {
            CountryCellId cellId = CountryCellId::FromString(name);
            pair<int64_t, int> bl = cellId.ToBitsAndLevel();
            if (level < 0)
              level = bl.second;
            CHECK_EQUAL( level, bl.second, ("Data files with different level?", *it) );
            bits = static_cast<uint16_t>(bl.first);
            CHECK_EQUAL( name, CountryCellId::FromBitsAndLevel(bits, level).ToString(), (name));
            cellFiles.push_back(make_pair(bits, static_cast<uint32_t>(size)));
          }
          else
          {
            commonFiles.push_back(make_pair(*it, static_cast<uint32_t>(size)));
          }
        }
        else
        {
          commonFiles.push_back(make_pair(*it, static_cast<uint32_t>(size)));
        }
      }
    }

    SaveTiles(dataDir + UPDATE_CHECK_FILE, level, cellFiles, commonFiles);

    LOG_SHORT(LINFO, ("Created update file with", cellFiles.size(), "cell data files and",
                      commonFiles.size(), "other files"));

    return true;
  }
} // namespace update
