#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include <cstdint>
#include <vector>

class Framework;

namespace sailfish
{
// As {id, isTrack}, e.g. from BookmarksModel::items().
void SplitItems(QVariantList const & items, std::vector<uint64_t> & marks, std::vector<uint64_t> & tracks);

// The app-wide bookmarks service: the lists, deleting items, import, export for sharing, and backups.
class BookmarksIO : public QObject
{
  Q_OBJECT
  // {id, name} rows.
  Q_PROPERTY(QVariantList lists READ lists NOTIFY listsChanged)
  Q_PROPERTY(QStringList importFilters READ importFilters CONSTANT)
  // KMZ backups of all lists; only the newest ones are kept.
  Q_PROPERTY(QString backupFolder READ backupFolder WRITE setBackupFolder NOTIFY backupChanged)
  // Days, 0 disables automatic backups.
  Q_PROPERTY(int backupPeriod READ backupPeriod WRITE setBackupPeriod NOTIFY backupChanged)
  Q_PROPERTY(QDateTime lastBackup READ lastBackup NOTIFY backupChanged)
  Q_PROPERTY(bool backingUp READ backingUp NOTIFY backupChanged)

public:
  // The kml::FileType values offered for export.
  enum FileType
  {
    Kmz,
    Gpx,
    GeoJson
  };
  Q_ENUM(FileType)

  explicit BookmarksIO(Framework & framework, QObject * parent = nullptr);

  QStringList importFilters() const;
  QString backupFolder() const;
  void setBackupFolder(QString const & folder);
  int backupPeriod() const;
  void setBackupPeriod(int days);
  QDateTime lastBackup() const;
  bool backingUp() const { return m_backingUp; }
  QVariantList lists() const;

  // New bookmarks then go to the new list.
  Q_INVOKABLE quint64 createList(QString const & name);
  // Takes {id, isTrack} items; also closes a place page showing one of them.
  Q_INVOKABLE void deleteItems(QVariantList const & items);

  // Takes a path or a file:// URL.
  Q_INVOKABLE void importFile(QString const & file);
  // exportReady() or exportFailed() follows asynchronously.
  Q_INVOKABLE void exportList(quint64 listId, int fileType);
  Q_INVOKABLE void exportTrack(quint64 trackId, int fileType);
  Q_INVOKABLE void exportAll();
  Q_INVOKABLE void backUpNow();
  // List names are unique.
  Q_INVOKABLE bool isListNameTaken(QString const & name) const;

  // Returns false for files that can't be imported.
  bool OpenFile(QString const & file);
  void BackUpIfDue();

signals:
  void exportReady(QString const & fileUrl, QString const & mimeType);
  void exportFailed(QString const & message);
  void importFinished(bool success, QString const & message);
  void backupChanged();
  void backupFinished(bool success);
  void listsChanged();

private:
  void OnBackupFile(QString const & path);

  Framework & m_framework;
  bool m_backingUp = false;
};
}  // namespace sailfish
