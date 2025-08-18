#include "track_analyzing/temporary_file.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

using namespace std;

TemporaryFile::TemporaryFile() : m_filePath(GetPlatform().TmpPathForFile()) {}

TemporaryFile::TemporaryFile(std::string const & namePrefix, std::string const & nameSuffix)
  : m_filePath(GetPlatform().TmpPathForFile(namePrefix, nameSuffix))
{}

TemporaryFile::~TemporaryFile()
{
  Platform::RemoveFileIfExists(m_filePath);
}

void TemporaryFile::WriteData(string const & data)
{
  FileWriter writer(m_filePath);
  writer.Write(data.data(), data.size());
  writer.Flush();
}
