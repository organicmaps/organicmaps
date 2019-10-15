#include "map/notifications/notification_queue_storage.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

namespace notifications
{
// static
std::string QueueStorage::GetNotificationsDir()
{
  return base::JoinPath(GetPlatform().SettingsDir(), "notifications");
}

// static
std::string QueueStorage::GetFilePath()
{
  return base::JoinPath(GetNotificationsDir(), "queue");
}

// static
bool QueueStorage::Save(std::vector<int8_t> const & src)
{
  if (!GetPlatform().IsDirectory(GetNotificationsDir()) &&
      !GetPlatform().MkDirChecked(GetNotificationsDir()))
  {
    return false;
  }

  return base::WriteToTempAndRenameToFile(GetFilePath(), [&src](std::string const & fileName)
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

// static
bool QueueStorage::Load(std::vector<int8_t> & dst)
{
  try
  {
    FileReader reader(GetFilePath());

    dst.clear();
    dst.resize(static_cast<size_t>(reader.Size()));

    reader.Read(0, dst.data(), dst.size());
  }
  catch (FileReader::Exception const &)
  {
    dst.clear();
  }

  return !dst.empty();
}
}  // namespace notifications
