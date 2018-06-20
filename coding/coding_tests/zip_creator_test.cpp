#include "testing/testing.hpp"

#include "coding/zip_creator.hpp"
#include "coding/zip_reader.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/file_writer.hpp"
#include "coding/constants.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"


namespace
{

void CreateAndTestZip(string const & filePath, string const & zipPath)
{
  TEST(CreateZipFromPathDeflatedAndDefaultCompression(filePath, zipPath), ());

  ZipFileReader::FileListT files;
  ZipFileReader::FilesList(zipPath, files);
  TEST_EQUAL(files[0].second, FileReader(filePath).Size(), ());

  string const unzippedFile = "unzipped.tmp";
  ZipFileReader::UnzipFile(zipPath, files[0].first, unzippedFile);

  TEST(my::IsEqualFiles(filePath, unzippedFile), ());
  TEST(my::DeleteFileX(filePath), ());
  TEST(my::DeleteFileX(zipPath), ());
  TEST(my::DeleteFileX(unzippedFile), ());
}

}

UNIT_TEST(CreateZip_BigFile)
{
  string const name = "testfileforzip.txt";

  {
    FileWriter f(name);
    string s(READ_FILE_BUFFER_SIZE + 1, '1');
    f.Write(s.c_str(), s.size());
  }

  CreateAndTestZip(name, "testzip.zip");
}

UNIT_TEST(CreateZip_Smoke)
{
  string const name = "testfileforzip.txt";

  {
    FileWriter f(name);
    f.Write(name.c_str(), name.size());
  }

  CreateAndTestZip(name, "testzip.zip");
}
