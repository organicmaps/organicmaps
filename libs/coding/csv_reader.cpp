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

void CSVReader::ReaderWrapper::FillBuffer()
{
  uint64_t const remaining = m_reader.Size() - m_pos;
  size_t const toRead = std::min(static_cast<size_t>(remaining), kBufferSize);
  m_buf.resize(toRead);
  m_reader.Read(m_pos, m_buf.data(), toRead);
  m_pos += toRead;
  m_bufPos = 0;
  m_bufEnd = toRead;
}

std::optional<std::string> CSVReader::ReaderWrapper::ReadLine()
{
  if (m_bufPos >= m_bufEnd && m_pos >= m_reader.Size())
    return {};

  std::string line;
  for (;;)
  {
    if (m_bufPos >= m_bufEnd)
    {
      if (m_pos >= m_reader.Size())
        break;
      FillBuffer();
    }

    auto const * begin = m_buf.data() + m_bufPos;
    auto const * end = m_buf.data() + m_bufEnd;
    auto const * nl = std::find(begin, end, '\n');

    if (nl != end)
    {
      line.append(begin, nl);
      m_bufPos = static_cast<size_t>(nl - m_buf.data()) + 1;
      return line;
    }

    line.append(begin, end);
    m_bufPos = m_bufEnd;
  }

  if (line.empty())
    return {};

  return line;
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
