#include "../../testing/testing.hpp"

#include "../../map/framework.hpp"

#include "../../coding/zip_creator.hpp"
#include "../../coding/zip_reader.hpp"
#include "../../coding/internal/file_data.hpp"
#include "../../coding/writer.hpp"

#include "../../base/scope_guard.hpp"

#include "../../platform/platform.hpp"

#include "../../std/string.hpp"
#include "../../std/vector.hpp"
#include "../../std/iostream.hpp"

UNIT_TEST(Create_Zip_From_Big_File)
{
  string const name = "testfileforzip.txt";

  {
    my::FileData f(name, my::FileData::OP_WRITE_TRUNCATE);
    string z(1024*512 + 1, '1');
    f.Write(z.c_str(), z.size());
  }

  string const zipName = "testzip.zip";

  TEST(createZipFromPathDeflatedAndDefaultCompression(name, zipName), ());


  vector<string> files;
  ZipFileReader::FilesList(zipName, files);
  string const unzippedFile = "unzipped.tmp";
  ZipFileReader::UnzipFile(zipName, files[0], unzippedFile);


  TEST(my::IsEqualFiles(name, unzippedFile),());
  TEST(my::DeleteFileX(name), ());
  TEST(my::DeleteFileX(zipName), ());
  TEST(my::DeleteFileX(unzippedFile), ());
}

UNIT_TEST(Create_zip)
{
  string const name = "testfileforzip.txt";

  {
    my::FileData f(name, my::FileData::OP_WRITE_TRUNCATE);
    f.Write(name.c_str(), name.size());
  }

  string const zipName = "testzip.zip";

  TEST(createZipFromPathDeflatedAndDefaultCompression(name, zipName), ());


  vector<string> files;
  ZipFileReader::FilesList(zipName, files);
  string const unzippedFile = "unzipped.tmp";
  ZipFileReader::UnzipFile(zipName, files[0], unzippedFile);


  TEST(my::IsEqualFiles(name, unzippedFile),());
  TEST(my::DeleteFileX(name), ());
  TEST(my::DeleteFileX(zipName), ());
  TEST(my::DeleteFileX(unzippedFile), ());
}
