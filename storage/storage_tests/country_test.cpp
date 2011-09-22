#include "../../testing/testing.hpp"

#include "../country.hpp"

#include "../../version/version.hpp"

#include "../../coding/file_writer.hpp"
#include "../../coding/file_reader.hpp"

#include "../../base/start_mem_debug.hpp"

using namespace storage;


UNIT_TEST(TilesSerialization)
{
  static string const FILE = "tiles_serialization_test";
  TCommonFiles::value_type const vv1("str2", 456);
  TCommonFiles::value_type const vv2("str1", 123);
  {
    TCommonFiles commonFiles;
    commonFiles.push_back(vv1);
    commonFiles.push_back(vv2);

    SaveTiles(FILE, commonFiles);
  }

  {
    uint32_t version;

    TTilesContainer tiles;
    TEST(LoadTiles(ReaderPtr<Reader>(new FileReader(FILE)), tiles, version), ());

    TEST_EQUAL( tiles.size(), 2, ());
    TEST_EQUAL( tiles[0], TTilesContainer::value_type("str1", 123), ());
    TEST_EQUAL( tiles[1], TTilesContainer::value_type("str2", 456), ());
  }

  FileWriter::DeleteFileX(FILE);
}
