#include "indexer/transliteration_loader.hpp"

#include "platform/platform.hpp"

#include "coding/transliteration.hpp"
#include "coding/zip_reader.hpp"

#include "base/exception.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <string>

void InitTransliterationInstanceWithDefaultDirs()
{
  Platform const & pl = GetPlatform();

#if defined(OMIM_OS_ANDROID)
  char const kICUDataFile[] = "icudt75l.dat";
  if (!pl.IsFileExistsByFullPath(base::JoinPath(pl.WritableDir(), kICUDataFile)))
  {
    try
    {
      ZipFileReader::UnzipFile(pl.ResourcesDir(), std::string("assets/") + kICUDataFile,
                               pl.WritableDir() + kICUDataFile);
    }
    catch (RootException const & e)
    {
      LOG(LWARNING, ("Can't get transliteration data file \"", kICUDataFile, "\", reason:", e.Msg()));
    }
  }
  Transliteration::Instance().Init(pl.WritableDir());
#else
  Transliteration::Instance().Init(pl.ResourcesDir());
#endif
}
