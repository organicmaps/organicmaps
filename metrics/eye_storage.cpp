#include "metrics/eye_storage.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"

namespace
{
bool Save(std::string const & filename, std::vector<int8_t> const & src)
{
  return base::WriteToTempAndRenameToFile(filename, [&src](string const & fileName)
  {
    try
    {
      FileWriter writer(fileName);
      writer.Write(src.data(), src.size());
    }
    catch (FileWriter::Exception const & ex)
    {
      LOG(LERROR, (ex.what(), ex.Msg()));
      return false;
    }

    return true;
  });
}

bool Load(std::string const & filename, std::vector<int8_t> & dst)
{
  try
  {
    FileReader reader(filename);

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

bool Append(std::string const & filename, std::vector<int8_t> const & src)
{
  try
  {
    FileWriter writer(filename, FileWriter::Op::OP_APPEND);
    writer.Write(src.data(), src.size());
  }
  catch (FileWriter::Exception const &)
  {
    return false;
  }

  return true;
}
}  // namespace

namespace eye
{
// static
std::string Storage::GetEyeDir()
{
  return base::JoinPath(GetPlatform().SettingsDir(), "metric");
}

// static
std::string Storage::GetInfoFilePath()
{
  return base::JoinPath(GetEyeDir(), "info");
}

// static
std::string Storage::GetPoiEventsFilePath()
{
  return base::JoinPath(GetEyeDir(), "events");
}


// static
bool Storage::SaveInfo(std::vector<int8_t> const & src)
{
  return Save(GetInfoFilePath(), src);
}

// static
bool Storage::LoadInfo(std::vector<int8_t> & dst)
{
  return Load(GetInfoFilePath(), dst);
}

// static
bool Storage::SaveMapObjects(std::vector<int8_t> const & src)
{
  return Save(GetPoiEventsFilePath(), src);
}

// static
bool Storage::LoadMapObjects(std::vector<int8_t> & dst)
{
  return Load(GetPoiEventsFilePath(), dst);
}

// static
bool Storage::AppendMapObjectEvent(std::vector<int8_t> const & src)
{
  return Append(GetPoiEventsFilePath(), src);
}

// static
void Storage::Migrate()
{
  if (!GetPlatform().MkDirChecked(GetEyeDir()))
    return;

  auto const oldPath = GetPlatform().WritablePathForFile("metrics");
  if (!GetPlatform().IsFileExistsByFullPath(oldPath))
    return;

  if (GetPlatform().IsFileExistsByFullPath(GetInfoFilePath()))
  {
    base::DeleteFileX(oldPath);
    return;
  }

  if (!base::CopyFileX(oldPath, GetInfoFilePath()))
    return;

  base::DeleteFileX(oldPath);
}
}  // namespace eye
