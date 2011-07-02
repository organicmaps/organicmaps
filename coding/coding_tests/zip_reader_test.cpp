#include "../../testing/testing.hpp"

#include "../zip_reader.hpp"
#include "../file_writer.hpp"

#include "../../base/logging.hpp"
#include "../../base/macros.hpp"

char const zipBytes[] = "PK\003\004\n\0\0\0\0\0\222\226\342>\302\032"
"x\372\005\0\0\0\005\0\0\0\b\0\034\0te"
"st.txtUT\t\0\003\303>\017N\017"
"?\017Nux\v\0\001\004\365\001\0\0\004P\0"
"\0\0Test\nPK\001\002\036\003\n\0\0"
"\0\0\0\222\226\342>\302\032x\372\005\0\0\0\005"
"\0\0\0\b\0\030\0\0\0\0\0\0\0\0\0\244"
"\201\0\0\0\0test.txtUT\005"
"\0\003\303>\017Nux\v\0\001\004\365\001\0\0"
"\004P\0\0\0PK\005\006\0\0\0\0\001\0\001"
"\0N\0\0\0G\0\0\0\0\0";

UNIT_TEST(ZipReaderSmoke)
{
  string const ZIPFILE = "smoke_test.zip";
  {
    FileWriter f(ZIPFILE);
    f.Write(zipBytes, ARRAY_SIZE(zipBytes) - 1);
  }

  bool noException = true;
  try
  {
    ZipFileReader r(ZIPFILE, "test.txt");
    string s;
    r.ReadAsString(s);
    TEST_EQUAL(s, "Test\n", ("Invalid zip file contents"));
  }
  catch (FileReader::Exception const & e)
  {
    noException = false;
    LOG(LERROR, (e.what()));
  }
  TEST(noException, ("Unhandled exception"));

  // invalid zip
  noException = true;
  try
  {
    ZipFileReader r("some_nonexisting_filename", "test.txt");
  }
  catch (FileReader::Exception const &)
  {
    noException = false;
  }
  TEST(!noException, ());

  // invalid file inside zip
  noException = true;
  try
  {
    ZipFileReader r(ZIPFILE, "test");
  }
  catch (FileReader::Exception const &)
  {
    noException = false;
  }
  TEST(!noException, ());

  FileWriter::DeleteFileX(ZIPFILE);
}

