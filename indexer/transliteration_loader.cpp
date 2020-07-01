#include "indexer/transliteration_loader.hpp"

#include "platform/platform.hpp"

#include "coding/transliteration.hpp"
#include "coding/zip_reader.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"

#include <string>

void InitTransliterationInstanceWithDefaultDirs()
{
#if defined(OMIM_OS_ANDROID)
  char const kICUDataFile[] = "icudt57l.dat";
  if (!GetPlatform().IsFileExistsByFullPath(GetPlatform().WritableDir() + kICUDataFile))
  {
    try
    {
      ZipFileReader::UnzipFile(GetPlatform().ResourcesDir(), std::string("assets/") + kICUDataFile,
                               GetPlatform().WritableDir() + kICUDataFile);
    }
    catch (RootException const & e)
    {
      LOG(LWARNING,
          ("Can't get transliteration data file \"", kICUDataFile, "\", reason:", e.Msg()));
    }
  }
  Transliteration::Instance().Init(GetPlatform().WritableDir());
#else
  Transliteration::Instance().Init(GetPlatform().ResourcesDir());
#endif
}
