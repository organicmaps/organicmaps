#pragma once

#include "coding/reader.hpp"

#include <fstream>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace coding
{
class CSVReader
{
public:
  using Row = std::vector<std::string>;
  using Rows = std::vector<Row>;

  explicit CSVReader(std::string const & filename, bool hasHeader = false, char delimiter = ',');
  explicit CSVReader(std::istream & stream, bool hasHeader = false, char delimiter = ',');
  explicit CSVReader(Reader const & reader, bool hasHeader = false, char delimiter = ',');

  bool HasHeader() const;
  char GetDelimiter() const;

  Row const & GetHeader() const;
  std::optional<Row> ReadRow();
  Rows ReadAll();

  template <typename Fn>
  void ForEachRow(Fn && fn)
  {
    while (auto const optRow = ReadRow())
      fn(*optRow);
  }

  // The total number of lines read including the header. Count starts at 0.
  size_t GetCurrentLineNumber() const;

private:
  class ReaderInterface
  {
  public:
    virtual ~ReaderInterface() = default;

    virtual std::optional<std::string> ReadLine() = 0;
  };

  class IstreamWrapper : public ReaderInterface
  {
  public:
    explicit IstreamWrapper(std::istream & stream);

    // ReaderInterface overrides:
    std::optional<std::string> ReadLine() override;

  private:
    std::istream & m_stream;
  };

  class ReaderWrapper : public ReaderInterface
  {
  public:
    explicit ReaderWrapper(Reader const & reader);

    // ReaderInterface overrides:
    std::optional<std::string> ReadLine() override;

  private:
    size_t m_pos = 0;
    Reader const & m_reader;
  };

  class DefaultReader : public ReaderInterface
  {
  public:
    explicit DefaultReader(std::string const & filename);

    // ReaderInterface overrides:
    std::optional<std::string> ReadLine() override;

  private:
    std::ifstream m_stream;
  };

  explicit CSVReader(std::unique_ptr<ReaderInterface> reader, bool hasHeader, char delimiter);

  std::unique_ptr<ReaderInterface> m_reader;
  size_t m_currentLine = 0;
  bool m_hasHeader = false;
  char m_delimiter = ',';
  Row m_header;
};

class CSVRunner
{
public:
  explicit CSVRunner(CSVReader && reader);

  class Iterator
  {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type = CSVReader::Row;

    explicit Iterator(CSVReader & reader, bool isEnd = false);
    Iterator(Iterator const & other);
    Iterator & operator++();
    Iterator operator++(int);
    // Checks whether both this and other are equal. Two CSVReader iterators are equal if both of
    // them are end-of-file iterators or not and both of them refer to the same CSVReader.
    bool operator==(Iterator const & other) const;
    bool operator!=(Iterator const & other) const;
    CSVReader::Row & operator*();

  private:
    CSVReader & m_reader;
    std::optional<CSVReader::Row> m_current;
  };

  // Warning: It reads first line.
  Iterator begin();
  Iterator end();

private:
  CSVReader m_reader;
};
}  // namespace coding
