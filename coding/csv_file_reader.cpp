#include "coding/csv_file_reader.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <fstream>
#include <sstream>

namespace coding
{
using namespace std;

void CSVReader::ReadLineByLine(string const & filePath, LineByLineCallback const & fn,
                               Params const & params) const
{
  ifstream file(filePath);
  if (!file)
  {
    LOG(LERROR, ("File not found at path: ", filePath));
    return;
  }

  string line;
  bool readFirstLine = params.m_shouldReadHeader;
  while (getline(file, line))
  {
    vector<string> splitLine;
    strings::ParseCSVRow(line, params.m_delimiter, splitLine);
    if (!readFirstLine)
    {
      readFirstLine = true;
      continue;
    }
    fn(splitLine);
  }
}

void CSVReader::ReadFullFile(string const & filePath, FullFileCallback const & fn,
                             Params const & params) const
{
  vector<vector<string>> file;
  ReadLineByLine(filePath, [&file](vector<string> const & row) { file.emplace_back(row); }, params);
  fn(file);
}
}  // namespace coding
