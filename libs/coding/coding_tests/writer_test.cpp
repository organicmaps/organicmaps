#include "testing/testing.hpp"

#include "coding/buffered_file_writer.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using namespace std;

namespace
{
static char const kTestWriteStr[] = "01234567";

template <class WriterT>
void TestWrite(WriterT & writer)
{
  writer.Write("01", 2);  // "01"
  TEST_EQUAL(writer.Pos(), 2, ());
  writer.Write("x", 1);  // "01x"
  TEST_EQUAL(writer.Pos(), 3, ());
  writer.Write("3", 1);  // "01x3"
  TEST_EQUAL(writer.Pos(), 4, ());
  writer.Seek(2);
  TEST_EQUAL(writer.Pos(), 2, ());
  writer.Write("2", 1);  // "0123"
  TEST_EQUAL(writer.Pos(), 3, ());
  writer.Seek(7);
  TEST_EQUAL(writer.Pos(), 7, ());
  writer.Write("7", 1);  // "0123???7"
  TEST_EQUAL(writer.Pos(), 8, ());
  writer.Seek(4);
  TEST_EQUAL(writer.Pos(), 4, ());
  writer.Write("45", 2);  // "012345?7"
  writer.Write("6", 1);   // "01234567"
}
}  // namespace

UNIT_TEST(MemWriter_Smoke)
{
  vector<char> s;
  MemWriter<vector<char>> writer(s);
  TestWrite(writer);
  TEST_EQUAL(string(s.begin(), s.end()), kTestWriteStr, ());
}

UNIT_TEST(FileWriter_Smoke)
{
  char const fileName[] = "file_writer_smoke_test.tmp";
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
  FileWriter::DeleteFileX(fileName);
}

UNIT_TEST(SubWriter_MemWriter_Smoke)
{
  vector<char> s;
  MemWriter<vector<char>> writer(s);
  writer.Write("aa", 2);
  {
    SubWriter<MemWriter<vector<char>>> subWriter(writer);
    TestWrite(subWriter);
  }
  writer.Write("bb", 2);
  TEST_EQUAL(string(s.begin(), s.end()), "aa" + string(kTestWriteStr) + "bb", ());
}

UNIT_TEST(SubWriter_FileWriter_Smoke)
{
  char const fileName[] = "sub_file_writer_smoke_test.tmp";
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
  FileWriter::DeleteFileX(fileName);
}

UNIT_TEST(FileWriter_DeleteFile)
{
  char const fileName[] = "delete_file_test";
  {
    FileWriter writer(fileName);
    writer.Write("123", 3);
  }
  {
    FileReader reader(fileName);
    TEST_EQUAL(reader.Size(), 3, ());
  }
  FileWriter::DeleteFileX(fileName);
  try
  {
    FileReader reader(fileName);
    TEST(false, ("Exception should be thrown!"));
  }
  catch (FileReader::OpenException &)
  {}
}

UNIT_TEST(FileWriter_AppendAndOpenExisting)
{
  char const fileName[] = "append_openexisting_file_test";
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
  FileWriter::DeleteFileX(fileName);
}

size_t const CHUNK_SIZE = 1024;
size_t const CHUNKS_COUNT = 21;
string const TEST_STRING = "Some Test String";

void WriteTestData1(Writer & w)
{
  w.Seek(CHUNKS_COUNT * CHUNK_SIZE);
  w.Write(TEST_STRING.data(), TEST_STRING.size());
}

void WriteTestData2(Writer & w)
{
  char c[CHUNK_SIZE];
  for (size_t i = 1; i < CHUNKS_COUNT; i += 2)
  {
    for (size_t j = 0; j < ARRAY_SIZE(c); ++j)
      c[j] = i;
    w.Seek(i * CHUNK_SIZE);
    w.Write(&c[0], ARRAY_SIZE(c));
  }
  for (size_t i = 0; i < CHUNKS_COUNT; i += 2)
  {
    for (size_t j = 0; j < ARRAY_SIZE(c); ++j)
      c[j] = i;
    w.Seek(i * CHUNK_SIZE);
    w.Write(&c[0], ARRAY_SIZE(c));
  }
}

void ReadTestData(Reader & r)
{
  string s;
  r.ReadAsString(s);
  for (size_t i = 0; i < CHUNKS_COUNT; ++i)
    for (size_t j = 0; j < CHUNK_SIZE; ++j)
      TEST_EQUAL(s[i * CHUNK_SIZE + j], static_cast<char>(i), (i, j));
  string const sub = s.substr(CHUNKS_COUNT * CHUNK_SIZE);
  TEST_EQUAL(sub, TEST_STRING, (sub, TEST_STRING));
}
template <typename WriterType>
void WriteToFileAndTest()
{
  string const TEST_FILE = "FileWriter_Chunks.test";
  {
    WriterType fileWriter(TEST_FILE, FileWriter::OP_WRITE_TRUNCATE);
    WriteTestData1(fileWriter);
  }
  {
    WriterType fileWriter(TEST_FILE, FileWriter::OP_WRITE_EXISTING);
    WriteTestData2(fileWriter);
  }
  {
    FileReader r(TEST_FILE);
    ReadTestData(r);
  }
  FileWriter::DeleteFileX(TEST_FILE);
}

UNIT_TEST(FileWriter_Chunks)
{
  WriteToFileAndTest<FileWriter>();
}

UNIT_TEST(BufferedFileWriter_Smoke)
{
  WriteToFileAndTest<BufferedFileWriter>();
}

UNIT_TEST(MemWriter_Chunks)
{
  string buffer;
  {
    MemWriter<string> memWriter(buffer);
    WriteTestData1(memWriter);
  }
  {
    MemWriter<string> memWriter(buffer);
    WriteTestData2(memWriter);
  }
  {
    MemReader r(buffer.data(), buffer.size());
    ReadTestData(r);
  }
}
