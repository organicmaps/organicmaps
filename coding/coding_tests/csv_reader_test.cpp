#include "testing/testing.hpp"

#include "coding/csv_reader.hpp"
#include "coding/file_reader.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include <string>
#include <vector>

using coding::CSVReader;
using platform::tests_support::ScopedFile;

using Row = std::vector<std::string>;
using File = std::vector<Row>;

namespace
{
std::string const kCSV1 = "a,b,c,d\ne,f,g h";
std::string const kCSV2 = "a,b,cd a b, c";
std::string const kCSV3 = "";
}  // namespace

UNIT_TEST(CSVReaderSmoke)
{
  auto const fileName = "test.csv";
  ScopedFile sf(fileName, kCSV1);
  FileReader fileReader(sf.GetFullPath());
  CSVReader reader;
  reader.Read(fileReader, [](File const & file) {
    TEST_EQUAL(file.size(), 1, ());
    TEST_EQUAL(file[0].size(), 3, ());
    Row const firstRow = {"e", "f", "g h"};
    TEST_EQUAL(file[0], firstRow, ());
  });

  CSVReader::Params p;
  p.m_readHeader = true;
  reader.Read(fileReader,
              [](File const & file) {
                TEST_EQUAL(file.size(), 2, ());
                Row const headerRow = {"a", "b", "c", "d"};
                TEST_EQUAL(file[0], headerRow, ());
              },
              p);
}

UNIT_TEST(CSVReaderCustomDelimiter)
{
  auto const fileName = "test.csv";
  ScopedFile sf(fileName, kCSV2);
  FileReader fileReader(sf.GetFullPath());
  CSVReader reader;
  CSVReader::Params p;
  p.m_readHeader = true;
  p.m_delimiter = ' ';

  reader.Read(fileReader,
              [](Row const & row) {
                Row const firstRow = {"a,b,cd", "a", "b,", "c"};
                TEST_EQUAL(row, firstRow, ());
              },
              p);
}

UNIT_TEST(CSVReaderEmptyFile)
{
  auto const fileName = "test.csv";
  ScopedFile sf(fileName, kCSV2);
  FileReader fileReader(sf.GetFullPath());

  CSVReader reader;
  reader.Read(fileReader, [](File const & file) { TEST_EQUAL(file.size(), 0, ()); });
}
