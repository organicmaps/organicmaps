#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"

#include "reader_test.hpp"

#include "../file_reader.hpp"
#include "../file_writer.hpp"
#include "../buffer_reader.hpp"

namespace
{
  string const kData("Quick brown fox jumps over a lazy dog...");
}

UNIT_TEST(MemReaderSmokeTest)
{
  MemReader memReader(kData.c_str(), kData.size());
  TestReader(memReader);
}

UNIT_TEST(FileReaderSmokeTest)
{
  {
    FileWriter writer("reader_test_tmp.dat");
    writer.Write(&kData[0], kData.size());
  }

  {
    FileReader fileReader("reader_test_tmp.dat");
    TestReader(fileReader);
  }
  FileWriter::DeleteFile("reader_test_tmp.dat");
}

UNIT_TEST(BufferReaderSmokeTest)
{
  BufferReader r1(&kData[0], kData.size());
  TestReader(r1);

  {
    string const data("BlaBla " + kData);
    FileWriter writer("reader_test_tmp.dat");
    writer.Write(&data[0], data.size());
  }

  BufferReader r2(FileReader("reader_test_tmp.dat"), 7);
  TestReader(r2);
  FileWriter::DeleteFile("reader_test_tmp.dat");
}

UNIT_TEST(BufferReaderEmptyTest)
{
  MemReader reader(NULL, 0);
  BufferReader bufReader(reader, 0);
  TEST_EQUAL(bufReader.Size(), 0, ());
}

UNIT_TEST(FileReaderNonExistentFileTest)
{
  try
  {
    FileReader reader("skjhfaxniauiuq2bmnszmn093sklsd");
    TEST(false, ("Exception should be thrown!"));
  }
  catch (FileReader::OpenException &)
  {
  }
}

UNIT_TEST(FileReaderReadAsText)
{
  char const fName[] = "zzzuuuuuummmba";
  {
    FileWriter f(fName);
    f.Write(fName, ARRAY_SIZE(fName) - 1);
  }
  {
    FileReader f(fName);
    string const text = f.ReadAsText();
    TEST_EQUAL(text, fName, ());
  }
  FileWriter::DeleteFile(fName);
}
