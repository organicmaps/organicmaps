#include "../../testing/testing.hpp"
#include "../file_writer.hpp"
#include "../file_reader.hpp"

namespace
{
  static char const kTestWriteStr [] = "01234567";

  template <class WriterT>
  void TestWrite(WriterT & writer)
  {
    writer.Write("01", 2);           // "01"
    TEST_EQUAL(writer.Pos(), 2, ());
    writer.Write("x", 1);            // "01x"
    TEST_EQUAL(writer.Pos(), 3, ());
    writer.Write("3", 1);            // "01x3"
    TEST_EQUAL(writer.Pos(), 4, ());
    writer.Seek(2);
    TEST_EQUAL(writer.Pos(), 2, ());
    writer.Write("2", 1);            // "0123"
    TEST_EQUAL(writer.Pos(), 3, ());
    writer.Seek(7);
    TEST_EQUAL(writer.Pos(), 7, ());
    writer.Write("7", 1);            // "0123???7"
    TEST_EQUAL(writer.Pos(), 8, ());
    writer.Seek(4);
    TEST_EQUAL(writer.Pos(), 4, ());
    writer.Write("45", 2);           // "012345?7"
    writer.Write("6", 1);            // "01234567"
  }
}

UNIT_TEST(MemWriter_Smoke)
{
  vector<char> s;
  MemWriter<vector<char> > writer(s);
  TestWrite(writer);
  TEST_EQUAL(string(s.begin(), s.end()), kTestWriteStr, ());
}

UNIT_TEST(FileWriter_Smoke)
{
  char const fileName [] = "file_writer_smoke_test.tmp";
  {
    FileWriter writer(fileName);
    TestWrite(writer);
  }
  vector<char> s;
  {
    FileReader reader(fileName);
    s.resize(reader.Size());
    reader.Read(0, &s[0], reader.Size());
  }
  TEST_EQUAL(string(s.begin(), s.end()), kTestWriteStr, ());
  FileWriter::DeleteFile(fileName);
}

UNIT_TEST(SubWriter_MemWriter_Smoke)
{
  vector<char> s;
  MemWriter<vector<char> > writer(s);
  writer.Write("aa", 2);
  {
    SubWriter<MemWriter<vector<char> > > subWriter(writer);
    TestWrite(subWriter);
  }
  writer.Write("bb", 2);
  TEST_EQUAL(string(s.begin(), s.end()), "aa" + string(kTestWriteStr) + "bb", ());
}

UNIT_TEST(SubWriter_FileWriter_Smoke)
{
  char const fileName [] = "sub_file_writer_smoke_test.tmp";
  {
    FileWriter writer(fileName);
    writer.Write("aa", 2);
    {
      SubWriter<FileWriter> subWriter(writer);
      TestWrite(subWriter);
    }
    writer.Write("bb", 2);
  }
  vector<char> s;
  {
    FileReader reader(fileName);
    s.resize(reader.Size());
    reader.Read(0, &s[0], reader.Size());
  }
  TEST_EQUAL(string(s.begin(), s.end()), "aa" + string(kTestWriteStr) + "bb", ());
  FileWriter::DeleteFile(fileName);
}

UNIT_TEST(FileWriter_DeleteFile)
{
  char const fileName [] = "delete_file_test";
  {
    FileWriter writer(fileName);
    writer.Write("123", 3);
  }
  {
    FileReader reader(fileName);
    TEST_EQUAL(reader.Size(), 3, ());
  }
  FileWriter::DeleteFile(fileName);
  try
  {
    FileReader reader(fileName);
    TEST(false, ("Exception should be thrown!"));
  }
  catch (FileReader::OpenException & )
  {
  }
}

UNIT_TEST(FileWriter_AppendAndOpenExisting)
{
  char const fileName [] = "append_openexisting_file_test";
  {
    FileWriter writer(fileName);
  }
  {
    FileWriter writer(fileName, FileWriter::OP_WRITE_EXISTING);
    TEST_EQUAL(writer.Size(), 0, ());
    writer.Write("abcd", 4);
  }
  {
    FileReader reader(fileName);
    TEST_EQUAL(reader.Size(), 4, ());
    string s(static_cast<uint32_t>(reader.Size()), 0);
    reader.Read(0, &s[0], s.size());
    TEST_EQUAL(s, "abcd", ());
  }
  {
    FileWriter writer(fileName);
    writer.Write("123", 3);
  }
  {
    FileReader reader(fileName);
    TEST_EQUAL(reader.Size(), 3, ());
  }
  {
    FileWriter writer(fileName, FileWriter::OP_APPEND);
    writer.Write("4", 1);
  }
  {
    FileReader reader(fileName);
    TEST_EQUAL(reader.Size(), 4, ());
    string s(static_cast<uint32_t>(reader.Size()), 0);
    reader.Read(0, &s[0], s.size());
    TEST_EQUAL(s, "1234", ());
  }
  {
    FileWriter writer(fileName, FileWriter::OP_WRITE_EXISTING);
    TEST_EQUAL(writer.Size(), 4, ());
    writer.Write("56", 2);
  }
  {
    FileReader reader(fileName);
    TEST_EQUAL(reader.Size(), 4, ());
    string s(static_cast<uint32_t>(reader.Size()), 0);
    reader.Read(0, &s[0], 4);
    TEST_EQUAL(s, "5634", ());
  }
  FileWriter::DeleteFile(fileName);
}
