#include "metrics/eye_storage.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"

#include <type_traits>

namespace eye
{
// static
std::string Storage::GetEyeFilePath()
{
  return GetPlatform().WritablePathForFile("eye");
}

// static
bool Storage::Save(std::string const & filePath, std::vector<int8_t> const & src)
{
  return my::WriteToTempAndRenameToFile(filePath, [&src](string const & fileName)
  {
    try
    {
      FileWriter writer(fileName);
      writer.Write(src.data(), src.size());
    }
    catch (FileWriter::Exception const &)
    {
      return false;
    }

    return true;
  });
}

// static
bool Storage::Load(std::string const & filePath, std::vector<int8_t> & dst)
{
  try
  {
    FileReader reader(filePath);

    dst.clear();
    dst.resize(reader.Size());

    reader.Read(0, dst.data(), dst.size());
  }
  catch (FileReader::Exception const &)
  {
    dst.clear();
    return false;
  }

  return true;
}
}  // namespace eye
