#include "coding/csv_reader.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

namespace coding
{
using namespace std;

void CSVReader::Read(istringstream & stream, RowByRowCallback const & fn,
                     Params const & params) const
{
  bool readFirstRow = params.m_readHeader;

  for (string line; getline(stream, line);)
  {
    Row row;
    strings::ParseCSVRow(line, params.m_delimiter, row);
    if (!readFirstRow)
    {
      readFirstRow = true;
      continue;
    }
    fn(move(row));
  }
}

void CSVReader::Read(istringstream & stream, FullFileCallback const & fn,
                     Params const & params) const
{
  File file;
  Read(stream, [&file](Row && row)
  {
    file.emplace_back(move(row));
  }, params);

  fn(move(file));
}
}  // namespace coding
