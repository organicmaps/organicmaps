#pragma once

#include <string>

class TemporaryFile
{
public:
  TemporaryFile();
  TemporaryFile(std::string const & namePrefix, std::string const & nameSuffix);

  TemporaryFile(TemporaryFile const &) = delete;
  TemporaryFile & operator=(TemporaryFile const &) = delete;

  ~TemporaryFile();

  std::string const & GetFilePath() const { return m_filePath; }

  void WriteData(std::string const & data);

private:
  std::string m_filePath;
};
