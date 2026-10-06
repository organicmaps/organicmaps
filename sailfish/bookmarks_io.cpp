#include "sailfish/bookmarks_io.hpp"

#include "sailfish/bookmarks_model.hpp"
#include "sailfish/helpers.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"

#include "kml/type_utils.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QStandardPaths>
#include <QUrl>
#include <QVariantMap>

#include <array>
#include <functional>
#include <string_view>

namespace sailfish
{
namespace
{
std::string_view constexpr kBackupPeriodSetting = "SailfishBackupPeriod";
std::string_view constexpr kLastBackupSetting = "SailfishLastBackup";
std::string_view constexpr kBackupFolderSetting = "SailfishBackupFolder";
int constexpr kDefaultBackupPeriodDays = 7;
int constexpr kBackupsToKeep = 10;
auto constexpr kImportExtensions =
    std::to_array({kKmzExtension, kKmlExtension, kKmbExtension, kGpxExtension, kGeoJsonExtension, kJsonExtension});

QString LocalPath(QString const & file)
{
  QUrl const url(file);
  return url.isLocalFile() ? url.toLocalFile() : file;
}

bool IsImportable(QString const & path)
{
  for (auto const ext : kImportExtensions)
    if (path.endsWith(ToQString(ext), Qt::CaseInsensitive))
      return true;
  return false;
}
}  // namespace

void SplitItems(QVariantList const & items, std::vector<uint64_t> & marks, std::vector<uint64_t> & tracks)
{
  for (auto const & item : items)
  {
    auto const map = item.toMap();
    (map["isTrack"].toBool() ? tracks : marks).push_back(map["id"].toULongLong());
  }
}

BookmarksIO::BookmarksIO(Framework & framework, QObject * parent) : QObject(parent), m_framework(framework)
{
  auto & notifier = BookmarksNotifier::Instance();
  connect(&notifier, &BookmarksNotifier::fileLoaded, this, [this](QString const & path, bool success)
  {
    QString const name = QFileInfo(path).fileName();
    emit importFinished(success, success ? Localized("load_kmz_successful", {name}) : Localized("load_kmz_failed"));
  });
  connect(&notifier, &BookmarksNotifier::loaded, this, &BookmarksIO::BackUpIfDue);
  connect(&notifier, &BookmarksNotifier::changed, this, &BookmarksIO::listsChanged);
}

QVariantList BookmarksIO::lists() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  QVariantList lists;
  for (auto const id : manager.GetSortedBmGroupIdList())
  {
    lists.append(QVariantMap{{"id", QVariant::fromValue<quint64>(id)},
                             {"name", QString::fromStdString(manager.GetCategoryName(id))}});
  }
  return lists;
}

quint64 BookmarksIO::createList(QString const & name)
{
  auto & manager = m_framework.GetBookmarkManager();
  auto const id = manager.CreateBookmarkCategory(name.trimmed().toStdString());
  manager.SetLastEditedBmCategory(id);
  return id;
}

void BookmarksIO::deleteItems(QVariantList const & items)
{
  kml::MarkIdCollection marks;
  kml::TrackIdCollection tracks;
  SplitItems(items, marks, tracks);
  // A remorse timer runs this later, when some may be gone, which the core CHECKs.
  auto const & manager = m_framework.GetBookmarkManager();
  base::EraseIf(marks, [&manager](kml::MarkId id) { return !manager.HasBookmark(id); });
  base::EraseIf(tracks, [&manager](kml::TrackId id) { return !manager.HasTrack(id); });
  m_framework.DeleteBookmarksAndTracks(marks, tracks);
}

QStringList BookmarksIO::importFilters() const
{
  QStringList filters;
  for (auto const ext : kImportExtensions)
    filters.append('*' + ToQString(ext));
  return filters;
}

QString BookmarksIO::backupFolder() const
{
  std::string folder;
  if (settings::Get(kBackupFolderSetting, folder) && !folder.empty())
    return QString::fromStdString(folder);
  return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QStringLiteral("/Organic Maps");
}

void BookmarksIO::setBackupFolder(QString const & folder)
{
  settings::Set(kBackupFolderSetting, folder.toStdString());
  emit backupChanged();
}

int BookmarksIO::backupPeriod() const
{
  return LoadSetting(kBackupPeriodSetting, kDefaultBackupPeriodDays);
}

void BookmarksIO::setBackupPeriod(int days)
{
  settings::Set(kBackupPeriodSetting, days);
  emit backupChanged();
  BackUpIfDue();
}

QDateTime BookmarksIO::lastBackup() const
{
  int64_t seconds = 0;
  return settings::Get(kLastBackupSetting, seconds) ? QDateTime::fromMSecsSinceEpoch(seconds * 1000) : QDateTime();
}

void BookmarksIO::importFile(QString const & file)
{
  m_framework.GetBookmarkManager().LoadBookmark(LocalPath(file).toStdString(), false /* isTemporaryFile */);
}

bool BookmarksIO::OpenFile(QString const & file)
{
  QString const path = LocalPath(file);
  if (!QFileInfo(path).isFile() || !IsImportable(path))
    return false;
  importFile(path);
  return true;
}

namespace
{
// BookmarkManager calls sharing handlers on the File thread.
BookmarkManager::SharingHandler OnGuiThread(
    QPointer<BookmarksIO> io, std::function<void(BookmarksIO &, BookmarkManager::SharingResult const &)> f)
{
  return [io, f = std::move(f)](BookmarkManager::SharingResult const & result)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [io, f, result]
    {
      if (io)
        f(*io, result);
    });
  };
}

BookmarkManager::SharingHandler MakeSharingHandler(QPointer<BookmarksIO> io)
{
  return OnGuiThread(io, [](BookmarksIO & io, BookmarkManager::SharingResult const & result)
  {
    using Code = BookmarkManager::SharingResult::Code;
    switch (result.m_code)
    {
    case Code::Success:
      emit io.exportReady(QUrl::fromLocalFile(QString::fromStdString(result.m_sharingPath)).toString(),
                          QString::fromStdString(result.m_mimeType));
      break;
    case Code::EmptyCategory: emit io.exportFailed(Localized("bookmarks_error_title_share_empty")); break;
    default: emit io.exportFailed(Localized("dialog_routing_system_error")); break;
    }
  });
}

FileType ToFileType(int fileType)
{
  switch (fileType)
  {
  case BookmarksIO::Kmz: return FileType::Kml;
  case BookmarksIO::Gpx: return FileType::Gpx;
  case BookmarksIO::GeoJson: return FileType::GeoJson;
  }
  UNREACHABLE();
}
}  // namespace

void BookmarksIO::exportList(quint64 listId, int fileType)
{
  // A page can still show a list or track deleted meanwhile, which the core CHECKs.
  if (!m_framework.GetBookmarkManager().HasBmCategory(listId))
    return;
  m_framework.GetBookmarkManager().PrepareFileForSharing({listId}, MakeSharingHandler(this), ToFileType(fileType));
}

void BookmarksIO::exportTrack(quint64 trackId, int fileType)
{
  if (!m_framework.GetBookmarkManager().HasTrack(trackId))
    return;
  m_framework.GetBookmarkManager().PrepareTrackFileForSharing(trackId, MakeSharingHandler(this), ToFileType(fileType));
}

void BookmarksIO::exportAll()
{
  m_framework.GetBookmarkManager().PrepareAllFilesForSharing(MakeSharingHandler(this));
}

void BookmarksIO::backUpNow()
{
  auto & manager = m_framework.GetBookmarkManager();
  if (m_backingUp || manager.AreAllCategoriesEmpty())
    return;
  m_backingUp = true;
  emit backupChanged();
  manager.PrepareAllFilesForSharing(OnGuiThread(this,
                                                [](BookmarksIO & io, BookmarkManager::SharingResult const & result)
  {
    bool const success = result.m_code == BookmarkManager::SharingResult::Code::Success;
    io.OnBackupFile(success ? QString::fromStdString(result.m_sharingPath) : QString());
  }));
}

bool BookmarksIO::isListNameTaken(QString const & name) const
{
  // Names are trimmed when saved.
  return m_framework.GetBookmarkManager().IsUsedCategoryName(name.trimmed().toStdString());
}

void BookmarksIO::OnBackupFile(QString const & path)
{
  m_backingUp = false;
  QDir const dir(backupFolder());
  QString const target =
      dir.filePath(QStringLiteral("bookmarks-%1.kmz").arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")));
  bool const success = !path.isEmpty() && QDir().mkpath(dir.path()) && QFile::copy(path, target);
  if (!path.isEmpty())
    QFile::remove(path);
  if (success)
  {
    settings::Set(kLastBackupSetting, static_cast<int64_t>(QDateTime::currentMSecsSinceEpoch() / 1000));
    // The names sort by time.
    auto const backups = dir.entryList({QStringLiteral("bookmarks-*.kmz")}, QDir::Files, QDir::Name | QDir::Reversed);
    for (int i = kBackupsToKeep; i < backups.size(); ++i)
      QFile::remove(dir.filePath(backups[i]));
  }
  else
  {
    LOG(LWARNING, ("Bookmarks backup to", target.toStdString(), "failed"));
  }
  emit backupChanged();
  emit backupFinished(success);
}

void BookmarksIO::BackUpIfDue()
{
  int const days = backupPeriod();
  if (days <= 0)
    return;
  QDateTime const last = lastBackup();
  if (!last.isValid() || last.addDays(days) <= QDateTime::currentDateTime())
    backUpNow();
}
}  // namespace sailfish
