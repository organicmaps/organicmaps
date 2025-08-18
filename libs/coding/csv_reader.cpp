#include "coding/csv_reader.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

namespace coding
{
CSVReader::CSVReader(std::unique_ptr<ReaderInterface> reader, bool hasHeader, char delimiter)
  : m_reader(std::move(reader))
  , m_hasHeader(hasHeader)
  , m_delimiter(delimiter)
{
  if (!HasHeader())
    return;

  auto const row = ReadRow();
  if (row)
    m_header = *row;
}

CSVReader::CSVReader(std::string const & filename, bool hasHeader, char delimiter)
  : CSVReader(std::make_unique<DefaultReader>(filename), hasHeader, delimiter)
{}

CSVReader::CSVReader(std::istream & stream, bool hasHeader, char delimiter)
  : CSVReader(std::make_unique<IstreamWrapper>(stream), hasHeader, delimiter)
{}

CSVReader::CSVReader(Reader const & reader, bool hasHeader, char delimiter)
  : CSVReader(std::make_unique<ReaderWrapper>(reader), hasHeader, delimiter)
{}

bool CSVReader::HasHeader() const
{
  return m_hasHeader;
}

char CSVReader::GetDelimiter() const
{
  return m_delimiter;
}

CSVReader::Row const & CSVReader::GetHeader() const
{
  return m_header;
}

CSVReader::Rows CSVReader::ReadAll()
{
  Rows file;
  ForEachRow([&](auto const & row) { file.emplace_back(row); });
  return file;
}

std::optional<CSVReader::Row> CSVReader::ReadRow()
{
  auto const line = m_reader->ReadLine();
  if (!line)
    return {};

  Row row;
  strings::ParseCSVRow(*line, m_delimiter, row);
  ++m_currentLine;
  return row;
}

size_t CSVReader::GetCurrentLineNumber() const
{
  return m_currentLine;
}

CSVReader::IstreamWrapper::IstreamWrapper(std::istream & stream) : m_stream(stream) {}

std::optional<std::string> CSVReader::IstreamWrapper::ReadLine()
{
  std::string line;
  return std::getline(m_stream, line) ? line : std::optional<std::string>();
}

CSVReader::ReaderWrapper::ReaderWrapper(Reader const & reader) : m_reader(reader) {}

std::optional<std::string> CSVReader::ReaderWrapper::ReadLine()
{
  std::vector<char> line;
  char ch = '\0';
  while (m_pos < m_reader.Size() && ch != '\n')
  {
    m_reader.Read(m_pos, &ch, sizeof(ch));
    line.emplace_back(ch);
    ++m_pos;
  }

  if (line.empty())
    return {};

  auto end = std::end(line);
  if (line.back() == '\n')
    --end;

  return std::string(std::begin(line), end);
}

CSVReader::DefaultReader::DefaultReader(std::string const & filename) : m_stream(filename)
{
  if (!m_stream)
    LOG(LERROR, ("Can't open file ", filename));

  m_stream.exceptions(std::ios::badbit);
}

std::optional<std::string> CSVReader::DefaultReader::ReadLine()
{
  return IstreamWrapper(m_stream).ReadLine();
}

CSVRunner::Iterator::Iterator(CSVReader & reader, bool isEnd) : m_reader(reader)
{
  if (!isEnd)
    m_current = m_reader.ReadRow();
}

CSVRunner::Iterator::Iterator(Iterator const & other) : m_reader(other.m_reader), m_current(other.m_current) {}

CSVRunner::Iterator & CSVRunner::Iterator::operator++()
{
  m_current = m_reader.ReadRow();
  return *this;
}

CSVRunner::Iterator CSVRunner::Iterator::operator++(int)
{
  Iterator tmp(*this);
  operator++();
  return tmp;
}

bool CSVRunner::Iterator::operator==(Iterator const & other) const
{
  return &m_reader == &other.m_reader && static_cast<bool>(m_current) == static_cast<bool>(other.m_current);
}

bool CSVRunner::Iterator::operator!=(Iterator const & other) const
{
  return !(*this == other);
}

CSVReader::Row & CSVRunner::Iterator::operator*()
{
  return *m_current;
}

CSVRunner::CSVRunner(CSVReader && reader) : m_reader(std::move(reader)) {}

CSVRunner::Iterator CSVRunner::begin()
{
  return Iterator(m_reader);
}

CSVRunner::Iterator CSVRunner::end()
{
  return Iterator(m_reader, true /* isEnd */);
}
}  // namespace coding
