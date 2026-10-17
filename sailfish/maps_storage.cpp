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
// An interrupted move leaves files in this folder that are also in the kept one.
char constexpr kCleanupKey[] = "moveCleanup";
char constexpr kKeptKey[] = "moveKept";

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

using FilePairs = std::vector<std::pair<std::string, std::string>>;

// Every file of the folder with its place in the new one, keeping the tree; none if one is there already.
std::optional<FilePairs> ListTree(QString const & from, QString const & to)
{
  FilePairs files;
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
  }
  return files;
}

// On failure the copies are removed.
bool CopyFiles(FilePairs const & files, std::function<void(double)> const & onProgress)
{
  qint64 total = 0;
  for (auto const & file : files)
    total += QFileInfo(QString::fromStdString(file.first)).size();
  qint64 done = 0;
  for (size_t i = 0; i < files.size(); ++i)
  {
    auto const & [source, target] = files[i];
    if (!QDir().mkpath(QFileInfo(QString::fromStdString(target)).path()) || !base::CopyFileX(source, target))
    {
      LOG(LWARNING, ("Can't copy", source, "to", target));
      for (size_t j = 0; j < i; ++j)
        base::DeleteFileX(files[j].second);
      return false;
    }
    done += QFileInfo(QString::fromStdString(target)).size();
    if (total > 0)
      onProgress(static_cast<double>(done) / total);
  }
  return true;
}

// Deletes the files of the folder that the kept one has too.
void RemoveDuplicates(QString const & dir, QString const & kept)
{
  for (QDirIterator it(dir, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories); it.hasNext();)
  {
    QString const file = it.next();
    if (QFileInfo::exists(QDir(kept).filePath(QDir(dir).relativeFilePath(file))))
      base::DeleteFileX(file.toStdString());
  }
}

// Saved before files are copied or deleted: the folder an interruption would leave duplicates in.
bool SetCleanup(QSettings & settings, QString const & dir, QString const & kept)
{
  settings.setValue(kCleanupKey, dir);
  settings.setValue(kKeptKey, kept);
  settings.sync();
  return settings.status() == QSettings::NoError;
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
  if (QString const cleanup = settings.value(kCleanupKey).toString(); !cleanup.isEmpty())
  {
    LOG(LWARNING, ("Removing what an interrupted maps move left in", cleanup.toStdString()));
    RemoveDuplicates(cleanup, settings.value(kKeptKey).toString());
    SetCleanup(settings, {}, {});
  }

  QString const configured = settings.value(kStorageKey).toString();
  QString dir = configured;
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
  auto const files = ListTree(from, to);
  // An interruption while copying leaves copies only, which the next start removes.
  if (!files || !SetCleanup(settings, to, from))
    return dir;
  if (!CopyFiles(*files, onProgress))
  {
    SetCleanup(settings, {}, {});
    return dir;
  }

  // The originals go only after the new folder is saved, so that an interruption leaves usable maps.
  QString const newDir = to == QDir::cleanPath(DefaultDir()) ? QString() : to;
  settings.setValue(kStorageKey, newDir);
  if (!SetCleanup(settings, from, to))
  {
    LOG(LERROR, ("Can't save the maps folder", to.toStdString(), "to", SettingsPath().toStdString()));
    settings.setValue(kStorageKey, configured);
    RemoveDuplicates(to, from);
    SetCleanup(settings, {}, {});
    return dir;
  }
  RemoveDuplicates(from, to);
  SetCleanup(settings, {}, {});
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
