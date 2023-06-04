#include "testing/testing.hpp"

#include "coding/csv_reader.hpp"
#include "coding/file_reader.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include <string>
#include <vector>

using platform::tests_support::ScopedFile;

using Row = coding::CSVReader::Row;
using Rows = coding::CSVReader::Rows;

namespace
{
std::string const kCSV1 = "a,b,c,d\ne,f,g h";
std::string const kCSV2 = "a,b,cd a b, c";
std::string const kCSV3 = "";
std::string const kCSV4 = "1,2\n3,4\n5,6";
std::string const kCSV5 = "1,2\n3,4\n\n5,6\n";
}  // namespace

UNIT_TEST(CSVReaderSmoke)
{
  auto const fileName = "test.csv";
  ScopedFile sf(fileName, kCSV1);
  {
    FileReader fileReader(sf.GetFullPath());
    coding::CSVReader reader(fileReader, false /* hasHeader */);
    auto const file = reader.ReadAll();
    TEST_EQUAL(file.size(), 2, ());
    Row const firstRow = {"a", "b", "c", "d"};
    TEST_EQUAL(file[0], firstRow, ());
    Row const secondRow = {"e", "f", "g h"};
    TEST_EQUAL(file[1], secondRow, ());
  }
  {
    FileReader fileReader(sf.GetFullPath());
    coding::CSVReader reader(fileReader, true /* hasHeader */);
    auto const file = reader.ReadAll();
    TEST_EQUAL(file.size(), 1, ());
    Row const headerRow = {"a", "b", "c", "d"};
    TEST_EQUAL(reader.GetHeader(), headerRow, ());
  }
}

UNIT_TEST(CSVReaderReadLine)
{
  auto const fileName = "test.csv";
  ScopedFile sf(fileName, kCSV4);
  Rows const answer = {{"1", "2"}, {"3", "4"}, {"5", "6"}};
  coding::CSVReader reader(sf.GetFullPath());
  size_t index = 0;
  while (auto const optionalRow = reader.ReadRow())
  {
    TEST_EQUAL(*optionalRow, answer[index], ());
    ++index;
  }
  TEST_EQUAL(index, answer.size(), ());
  TEST(!reader.ReadRow(), ());
  TEST(!reader.ReadRow(), ());
}

UNIT_TEST(CSVReaderCustomDelimiter)
{
  auto const fileName = "test.csv";
  ScopedFile sf(fileName, kCSV2);
  FileReader fileReader(sf.GetFullPath());
  coding::CSVReader reader(fileReader, false /* hasHeader */, ' ');
  auto const file = reader.ReadAll();
  TEST_EQUAL(file.size(), 1, ());
  Row const firstRow = {"a,b,cd", "a", "b,", "c"};
  TEST_EQUAL(file[0], firstRow, ());
}

UNIT_TEST(CSVReaderEmptyFile)
{
  auto const fileName = "test.csv";
  ScopedFile sf(fileName, kCSV3);
  FileReader fileReader(sf.GetFullPath());

  coding::CSVReader reader(fileReader);
  auto const file = reader.ReadAll();
  TEST_EQUAL(file.size(), 0, ());
}

UNIT_TEST(CSVReaderDifferentReaders)
{
  auto const fileName = "test.csv";
  ScopedFile sf(fileName, kCSV4);
  Rows const answer = {{"1", "2"}, {"3", "4"}, {"5", "6"}};
  {
    FileReader fileReader(sf.GetFullPath());
    coding::CSVReader reader(fileReader);
    auto const file = reader.ReadAll();
    TEST_EQUAL(file, answer, ());
  }
  {
    coding::CSVReader reader(sf.GetFullPath());
    auto const file = reader.ReadAll();
    TEST_EQUAL(file, answer, ());
  }
  {
    std::ifstream stream(sf.GetFullPath());
    coding::CSVReader reader(stream);
    auto const file = reader.ReadAll();
    TEST_EQUAL(file, answer, ());
  }
}

UNIT_TEST(CSVReaderEmptyLines)
{
  auto const fileName = "test.csv";
  ScopedFile sf(fileName, kCSV5);
  Rows const answer = {{"1", "2"}, {"3", "4"}, {}, {"5", "6"}};
  {
    FileReader fileReader(sf.GetFullPath());
    coding::CSVReader reader(fileReader);
    auto const file = reader.ReadAll();
    TEST_EQUAL(file, answer, ());
  }
  {
    coding::CSVReader reader(sf.GetFullPath());
    auto const file = reader.ReadAll();
    TEST_EQUAL(file, answer, ());
  }
  {
    std::ifstream stream(sf.GetFullPath());
    coding::CSVReader reader(stream);
    auto const file = reader.ReadAll();
    TEST_EQUAL(file, answer, ());
  }
}

UNIT_TEST(CSVReaderForEachRow)
{
  auto const fileName = "test.csv";
  ScopedFile sf(fileName, kCSV4);
  Rows const answer = {{"1", "2"}, {"3", "4"}, {"5", "6"}};
  FileReader fileReader(sf.GetFullPath());
  auto reader = coding::CSVReader(fileReader);
  size_t index = 0;
  reader.ForEachRow([&](auto const & row) {
    TEST_EQUAL(row, answer[index], ());
    ++index;
  });
  TEST_EQUAL(answer.size(), index, ());
}

UNIT_TEST(CSVReaderIterator)
{
  auto const fileName = "test.csv";
  ScopedFile sf(fileName, kCSV4);
  Rows const answer = {{"1", "2"}, {"3", "4"}, {"5", "6"}};
  {
    FileReader fileReader(sf.GetFullPath());
    coding::CSVRunner runner((coding::CSVReader(fileReader)));

    auto it = runner.begin();
    TEST_EQUAL(*it, answer[0], ());
    ++it;
    TEST_EQUAL(*it, answer[1], ());
    auto it2 = it++;
    TEST(it2 == it, ());
    TEST_EQUAL(*it2, answer[1], ());
    TEST_EQUAL(*it, answer[2], ());
    ++it;
    TEST(it == runner.end(), ());
  }
  {
    size_t index = 0;
    for (auto const & row : coding::CSVRunner(coding::CSVReader(sf.GetFullPath())))
    {
      TEST_EQUAL(row, answer[index], ());
      ++index;
    }
    TEST_EQUAL(index, answer.size(), ());
  }
}
