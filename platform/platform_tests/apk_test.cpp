#include "../../testing/testing.hpp"

#include "../platform.hpp"

#include "../../coding/zip_reader.hpp"
#include "../../coding/internal/file_data.hpp"

#include "../../base/thread.hpp"

#include "../../std/numeric.hpp"


namespace
{
  char const * arrFiles[] = {
    "about.html",
    "resources-ldpi/basic.skn",
    "resources-ldpi/plus.png",
    "resources-ldpi/symbols.png",
    "resources-mdpi/basic.skn",
    "resources-mdpi/plus.png",
    "resources-mdpi/symbols.png",
    "resources-hdpi/basic.skn",
    "resources-hdpi/plus.png",
    "resources-hdpi/symbols.png",
    "resources-xhdpi/basic.skn",
    "resources-xhdpi/plus.png",
    "resources-xhdpi/symbols.png",
    "categories.txt",
    "classificator.txt",
    "types.txt",
    "fonts_blacklist.txt",
    "fonts_whitelist.txt",
    "languages.txt",
    "unicode_blocks.txt",
    "drules_proto.bin",
    "external_resources.txt",
    "packed_polygons.bin",
    "countries.txt"
  };

  class ApkTester : public threads::IRoutine
  {
    static const int COUNT = ARRAY_SIZE(arrFiles);
    string const & m_cont;

  public:
    ApkTester(string const & cont) : m_cont(cont)
    {
      m_hashes.resize(COUNT);
    }

    virtual void Do()
    {
      string const prefix("assets/");

      while (true)
      {
        size_t ind = rand() % COUNT;
        if (m_hashes[ind] != 0)
        {
          ind = COUNT;
          for (size_t i = 0; i < COUNT; ++i)
            if (m_hashes[i] == 0)
            {
              ind = i;
              break;
            }
        }

        if (ind == COUNT)
          break;

        try
        {
          ZipFileReader reader(m_cont, prefix + arrFiles[ind]);

          size_t const size = reader.Size();
          vector<char> buffer(size);
          reader.Read(0, &buffer[0], size);

          m_hashes[ind] = accumulate(buffer.begin(), buffer.end(), static_cast<uint64_t>(0));
        }
        catch (Reader::Exception const & ex)
        {
          LOG(LERROR, (ex.Msg()));
        }
      }
    }

    vector<uint64_t> m_hashes;
  };
}

UNIT_TEST(ApkReader_Multithreaded)
{
  string const path = GetPlatform().WritableDir() + "../android/MapsWithMePro/bin/MapsWithMePro-production.apk";

  uint64_t size;
  if (!my::GetFileSize(path, size))
  {
    LOG(LINFO, ("Apk not found"));
    return;
  }

  srand(static_cast<unsigned>(size));

  size_t const count = 20;
  threads::ThreadPool pool(count);

  for (size_t i = 0; i < count; ++i)
    pool.Add(new ApkTester(path));

  pool.Join();

  typedef ApkTester const * PtrT;
  PtrT etalon = dynamic_cast<PtrT>(pool.GetRoutine(0));
  for (size_t i = 1; i < count; ++i)
  {
    PtrT p = dynamic_cast<PtrT>(pool.GetRoutine(i));
    TEST_EQUAL(etalon->m_hashes, p->m_hashes, ());
  }
}
