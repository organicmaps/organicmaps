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
  int32_t const level = 8;
  TDataFiles::value_type const v1(55000, 55000);
  TDataFiles::value_type const v2(15000, 15000);
  TDataFiles::value_type const v3(5, 5);
  TCommonFiles::value_type const vv1("str2", 456);
  TCommonFiles::value_type const vv2("str1", 123);
  {
    TDataFiles dataFiles;
    dataFiles.push_back(v1);
    dataFiles.push_back(v2);
    dataFiles.push_back(v3);
    TCommonFiles commonFiles;
    commonFiles.push_back(vv1);
    commonFiles.push_back(vv2);

    SaveTiles(FILE, level, dataFiles, commonFiles);
  }

  {
    uint32_t version;

    TTilesContainer tiles;
    TEST( LoadTiles(tiles, FILE, version), ());

    TEST_EQUAL( tiles.size(), 5, ());
    TEST_EQUAL( tiles[0], TTilesContainer::value_type(
        CountryCellId::FromBitsAndLevel(5, level).ToString(), 5), ());
    TEST_EQUAL( tiles[1], TTilesContainer::value_type(
        CountryCellId::FromBitsAndLevel(15000, level).ToString(), 15000), ());
    TEST_EQUAL( tiles[2], TTilesContainer::value_type(
        CountryCellId::FromBitsAndLevel(55000, level).ToString(), 55000), ());
    TEST_EQUAL( tiles[3], TTilesContainer::value_type("str1", 123), ());
    TEST_EQUAL( tiles[4], TTilesContainer::value_type("str2", 456), ());
  }

  FileWriter::DeleteFileX(FILE);
}
