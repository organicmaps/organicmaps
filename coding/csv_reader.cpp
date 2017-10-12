#include "coding/csv_reader.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

namespace coding
{
using namespace std;

void CSVReader::Read(istringstream & stream, LineByLineCallback const & fn,
                     Params const & params) const
{
  bool readFirstLine = params.m_readHeader;

  for (string line; getline(stream, line);)
  {
    Line splitLine;
    strings::ParseCSVRow(line, params.m_delimiter, splitLine);
    if (!readFirstLine)
    {
      readFirstLine = true;
      continue;
    }
    fn(move(splitLine));
  }
}

void CSVReader::Read(istringstream & stream, FullFileCallback const & fn,
                     Params const & params) const
{
  File file;
  Read(stream, [&file](Line && line)
  {
    file.emplace_back(move(line));
  }, params);

  fn(move(file));
}
}  // namespace coding
