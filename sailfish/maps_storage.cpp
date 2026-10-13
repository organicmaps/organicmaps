#include "sailfish/maps_storage.hpp"

#include "sailfish/countries_model.hpp"
#include "sailfish/helpers.hpp"

#include "map/framework.hpp"

#include "storage/storage.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QVariantMap>

#include <optional>
#include <vector>

namespace sailfish
{
namespace
{
char constexpr kStorageKey[] = "mapsStorage";
char constexpr kPendingKey[] = "pendingMove";

QString DefaultDir()
{
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

// QSettings() would use ~/.config/<org>/<app>.conf, outside the config folder Sailjail exposes.
QString SettingsPath()
{
  return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/maps_storage.conf");
}

QString CurrentDir()
{
  return QDir::cleanPath(QString::fromStdString(GetPlatform().WritableDir()));
}

// Copies every file of the folder, keeping the tree, and returns the copies; on failure they are removed.
std::optional<std::vector<std::string>> CopyTree(QString const & from, QString const & to,
                                                 std::function<void(double)> const & onProgress)
{
  std::vector<std::pair<std::string, std::string>> files;
  qint64 total = 0;
  for (QDirIterator it(from, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories); it.hasNext();)
  {
    QString const file = it.next();
    QString const target = QDir(to).filePath(QDir(from).relativeFilePath(file));
    // Never overwrite: e.g. edits.xml there is from the time the card was out and the app used its folder.
    if (QFileInfo::exists(target))
    {
      LOG(LWARNING, ("Not moving maps over the existing", target.toStdString()));
      return {};
    }
    files.emplace_back(file.toStdString(), target.toStdString());
    total += it.fileInfo().size();
  }

  std::vector<std::string> copied;
  qint64 done = 0;
  for (auto const & [source, target] : files)
  {
    if (!QDir().mkpath(QFileInfo(QString::fromStdString(target)).path()) || !base::CopyFileX(source, target))
    {
      LOG(LWARNING, ("Can't copy", source, "to", target));
      for (auto const & file : copied)
        base::DeleteFileX(file);
      return {};
    }
    copied.push_back(target);
    done += QFileInfo(QString::fromStdString(target)).size();
    if (total > 0)
      onProgress(static_cast<double>(done) / total);
  }
  return copied;
}
}  // namespace

bool MapsStorage::s_moveFailed = false;

MapsStorage::MapsStorage(Framework & framework, QObject * parent) : QObject(parent), m_framework(framework) {}

// static
bool MapsStorage::HasPendingMove()
{
  return !QSettings(SettingsPath(), QSettings::IniFormat).value(kPendingKey).toString().isEmpty();
}

// static
QString MapsStorage::ApplyPendingMove(std::function<void(double)> const & onProgress)
{
  QSettings settings(SettingsPath(), QSettings::IniFormat);
  QString dir = settings.value(kStorageKey).toString();
  // E.g. the memory card was removed: use the app folder until it is back.
  if (!dir.isEmpty() && !(QFileInfo(dir).isDir() && QFileInfo(dir).isWritable()))
    dir.clear();

  QString const pending = settings.value(kPendingKey).toString();
  if (pending.isEmpty())
    return dir;
  // Tried once: a failed or interrupted move leaves the maps where they are.
  settings.remove(kPendingKey);
  settings.sync();
  QString const from = dir.isEmpty() ? DefaultDir() : dir;
  QString const to = QDir::cleanPath(pending);
  LOG(LINFO, ("Moving maps from", from.toStdString(), "to", to.toStdString()));
  s_moveFailed = true;
  if (settings.status() != QSettings::NoError || !QDir().mkpath(to))
    return dir;
  auto const copies = CopyTree(from, to, onProgress);
  if (!copies)
    return dir;

  // The originals go only after the new folder is saved, so that an interruption leaves usable maps.
  QString const newDir = to == QDir::cleanPath(DefaultDir()) ? QString() : to;
  settings.setValue(kStorageKey, newDir);
  settings.sync();
  if (settings.status() != QSettings::NoError)
  {
    LOG(LERROR, ("Can't save the maps folder", to.toStdString(), "to", SettingsPath().toStdString()));
    for (auto const & file : *copies)
      base::DeleteFileX(file);
    return dir;
  }
  for (QDirIterator it(from, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories); it.hasNext();)
    base::DeleteFileX(it.next().toStdString());
  s_moveFailed = false;
  return newDir;
}

QVariantList MapsStorage::locations() const
{
  QString const current = CurrentDir();
  QVariantList result;
  auto const add = [&](QString const & path, QString const & name, QStorageInfo const & volume)
  {
    QString const details =
        Localized("maps_storage_free_size", {FormatSize(volume.bytesAvailable()), FormatSize(volume.bytesTotal())});
    result.append(QVariantMap{
        {"path", path}, {"name", name}, {"details", details}, {"current", QDir::cleanPath(path) == current}});
  };

  add(DefaultDir(), Localized("maps_storage_internal"), QStorageInfo(DefaultDir()));
  // Memory cards and USB sticks, mounted here by Sailfish OS.
  for (auto const & volume : QStorageInfo::mountedVolumes())
  {
    if (!volume.isValid() || !volume.isReady() || volume.isReadOnly() ||
        !volume.rootPath().startsWith(QStringLiteral("/run/media/")))
      continue;
    QString const name = volume.name().isEmpty() ? Localized("maps_storage_removable")
                                                 : Localized("maps_storage_removable") + " (" + volume.name() + ")";
    add(volume.rootPath() + QStringLiteral("/organicmaps"), name, volume);
  }
  return result;
}

QString MapsStorage::downloadedSize() const
{
  auto const & storage = m_framework.GetStorage();
  return FormatSize(static_cast<qint64>(MapAttrs(storage, storage.GetRootId()).m_localMwmSize));
}

QString MapsStorage::currentName() const
{
  for (auto const & location : locations())
  {
    auto const map = location.toMap();
    if (map["current"].toBool())
      return map["name"].toString();
  }
  return CurrentDir();
}

bool MapsStorage::moveTo(QString const & path)
{
  QString const to = QDir::cleanPath(path);
  QSettings settings(SettingsPath(), QSettings::IniFormat);
  settings.setValue(kPendingKey, to);
  settings.sync();
  if (settings.status() != QSettings::NoError)
  {
    LOG(LERROR, ("Can't save the maps move to", to.toStdString(), "in", SettingsPath().toStdString()));
    return false;
  }
  m_movePending = true;
  emit changed();
  return true;
}
}  // namespace sailfish
