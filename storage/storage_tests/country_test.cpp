#include "../../testing/testing.hpp"

#include "../country.hpp"

#include "../../version/version.hpp"

#include "../../coding/file_writer.hpp"
#include "../../coding/file_reader.hpp"

using namespace storage;

UNIT_TEST(FilesSerialization)
{
  static string const FILE = "tiles_serialization_test";
  CommonFilesT::value_type const vv1("str2", 456);
  CommonFilesT::value_type const vv2("str1", 123);
  {
    CommonFilesT commonFiles;
    commonFiles.push_back(vv1);
    commonFiles.push_back(vv2);

    SaveFiles(FILE, commonFiles);
  }

  {
    uint32_t version;

    FilesContainerT files;
    TEST(LoadFiles(ReaderPtr<Reader>(new FileReader(FILE)), files, version), ());

    TEST_EQUAL( files.size(), 2, ());
    TEST_EQUAL( files[0], FilesContainerT::value_type("str1", 123), ());
    TEST_EQUAL( files[1], FilesContainerT::value_type("str2", 456), ());
  }

  FileWriter::DeleteFileX(FILE);
}
