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

#include <vector>

namespace sailfish
{
namespace
{
char constexpr kStorageKey[] = "mapsStorage";

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

// Moves every file of the folder, keeping the tree; on failure the copies are removed and the folder is left.
bool MoveTree(QString const & from, QString const & to, std::atomic<bool> const & cancelled)
{
  std::vector<std::pair<std::string, std::string>> files;
  for (QDirIterator it(from, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories); it.hasNext();)
  {
    QString const file = it.next();
    QString const target = QDir(to).filePath(QDir(from).relativeFilePath(file));
    // Never overwrite: e.g. edits.xml there is from the time the card was out and the app used its folder.
    if (QFileInfo::exists(target))
    {
      LOG(LWARNING, ("Not moving maps over the existing", target.toStdString()));
      return false;
    }
    files.emplace_back(file.toStdString(), target.toStdString());
  }

  // Copy all first, so that a full card doesn't leave the maps split.
  std::vector<std::string> copied;
  for (auto const & [source, target] : files)
  {
    if (cancelled || !QDir().mkpath(QFileInfo(QString::fromStdString(target)).path()) ||
        !base::CopyFileX(source, target))
    {
      LOG(LWARNING, ("Can't copy", source, "to", target));
      for (auto const & file : copied)
        base::DeleteFileX(file);
      return false;
    }
    copied.push_back(target);
  }
  for (auto const & file : files)
    base::DeleteFileX(file.first);
  return true;
}
}  // namespace

std::atomic<bool> MapsStorage::s_locked = false;

MapsStorage::MapsStorage(Framework & framework, QObject * parent) : QObject(parent), m_framework(framework)
{
  connect(this, &MapsStorage::treeMoved, this, &MapsStorage::OnMoved, Qt::QueuedConnection);
}

MapsStorage::~MapsStorage()
{
  m_cancelled = true;
  if (m_thread.joinable())
    m_thread.join();
}

// static
QString MapsStorage::ConfiguredDir()
{
  QString const dir = QSettings(SettingsPath(), QSettings::IniFormat).value(kStorageKey).toString();
  if (dir.isEmpty())
    return {};
  // E.g. the memory card was removed: use the app folder until it is back.
  QFileInfo const info(dir);
  return info.isDir() && info.isWritable() ? dir : QString();
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

bool MapsStorage::canMove() const
{
  return !Locked() && !m_framework.GetStorage().IsDownloadInProgress() &&
         !m_framework.GetRoutingManager().IsRoutingActive() && !m_framework.IsTrackRecordingEnabled();
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
  if (!canMove())
    return false;
  QString const from = CurrentDir();
  QString const to = QDir::cleanPath(path);
  if (to == from)
    return true;
  if (!QDir().mkpath(to))
  {
    emit moveFinished(false);
    return true;
  }

  m_moving = true;
  s_locked = true;
  emit changed();
  m_framework.DeregisterAllMaps();
  if (m_thread.joinable())
    m_thread.join();
  m_thread = std::thread([this, from, to] { emit treeMoved(to, MoveTree(from, to, m_cancelled)); });
  return true;
}

void MapsStorage::OnMoved(QString const & path, bool success)
{
  m_moving = false;
  s_locked = success;
  m_moved = success;
  if (success)
  {
    QSettings settings(SettingsPath(), QSettings::IniFormat);
    settings.setValue(kStorageKey, path == QDir::cleanPath(DefaultDir()) ? QString() : path);
    settings.sync();
    if (settings.status() != QSettings::NoError)
      LOG(LERROR, ("Can't save the maps folder", path.toStdString(), "to", SettingsPath().toStdString()));
  }
  else
  {
    m_framework.RegisterAllMaps();
  }
  emit changed();
  emit moveFinished(success);
}
}  // namespace sailfish
