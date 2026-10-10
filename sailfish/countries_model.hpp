#pragma once

#include "storage/storage_defines.hpp"

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include <vector>

namespace downloader
{
struct Progress;
}

namespace storage
{
class Storage;
struct NodeAttrs;
}  // namespace storage

namespace sailfish
{
// The storage attributes of a map.
storage::NodeAttrs MapAttrs(storage::Storage const & storage, storage::CountryId const & countryId);
// {countryId, name, size, status, progress} of a map not on disk yet, or empty.
// withOutdated also includes an outdated map, with outdated set.
QVariantMap MissingMapInfo(storage::Storage const & storage, storage::CountryId const & countryId,
                           bool withOutdated = false);
void DownloadMap(storage::Storage & storage, storage::CountryId const & countryId);

// All map downloads, for the app wide UI: the download notification and the cover, the updates offered at start
// and counted in the menu, and the map of the position offered without maps. Available to QML as downloads.
class DownloadStatus : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool inProgress READ inProgress NOTIFY inProgressChanged)
  // The map being downloaded and its progress (0..1).
  Q_PROPERTY(QString downloadingName READ downloadingName NOTIFY progressChanged)
  Q_PROPERTY(double downloadingProgress READ downloadingProgress NOTIFY progressChanged)
  // MissingMapInfo() of the map at the position, empty without a position, offered while no map is downloaded.
  Q_PROPERTY(QVariantMap positionMap READ positionMap NOTIFY progressChanged)
  Q_PROPERTY(bool noMaps READ noMaps NOTIFY mapsChanged)
  // Downloaded maps with a newer version, and the download size of all their updates.
  Q_PROPERTY(int updateCount READ updateCount NOTIFY mapsChanged)
  Q_PROPERTY(QString updateSize READ updateSize NOTIFY mapsChanged)

public:
  explicit DownloadStatus(storage::Storage & storage, QObject * parent = nullptr);
  ~DownloadStatus() override;

  bool inProgress() const { return m_inProgress; }
  QString downloadingName() const { return m_downloadingName; }
  double downloadingProgress() const { return m_downloadingProgress; }
  QVariantMap positionMap() const;
  bool noMaps() const;
  int updateCount() const;
  QString updateSize() const;

  // True once per data version with updates.
  Q_INVOKABLE bool shouldOfferUpdate() const;
  Q_INVOKABLE void setUpdateOffered();
  Q_INVOKABLE void updateAll();
  Q_INVOKABLE void cancelAll();

signals:
  void inProgressChanged();
  void progressChanged();
  void mapsChanged();
  void downloadFailed(QString const & name);

private:
  void OnCountryChanged(storage::CountryId const & countryId);
  void OnProgress(storage::CountryId const & countryId, downloader::Progress const & progress);

  storage::Storage & m_storage;
  int m_storageSlot = 0;
  bool m_inProgress = false;
  storage::CountryId m_downloadingId;
  QString m_downloadingName;
  double m_downloadingProgress = 0;
};

class CountriesModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(QString parentId READ parentId WRITE setParentId NOTIFY parentIdChanged)
  Q_PROPERTY(QString title READ title NOTIFY parentIdChanged)
  // When unset, the maps left to download, with the regions around the position first at the root.
  Q_PROPERTY(bool downloadedOnly READ downloadedOnly WRITE setDownloadedOnly NOTIFY downloadedOnlyChanged)
  // Searches all maps by name when set.
  Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
  // The download size of the updates of the downloaded maps in the parent node.
  Q_PROPERTY(QString updateSize READ updateSize NOTIFY updatesChanged)
  // The status of the parent node, for the actions on all its maps.
  Q_PROPERTY(int parentStatus READ parentStatus NOTIFY updatesChanged)

public:
  // Mirrors storage::NodeStatus for QML.
  enum Status
  {
    Undefined,
    Downloading,
    Applying,
    InQueue,
    Error,
    OnDiskOutOfDate,
    OnDisk,
    NotDownloaded,
    Partly,
  };
  Q_ENUM(Status)

  // Mirrors storage::NodeErrorCode for QML.
  enum ErrorCode
  {
    NoError,
    UnknownError,
    OutOfMemFailed,
    NoInetConnection,
  };
  Q_ENUM(ErrorCode)

  enum Roles
  {
    CountryIdRole = Qt::UserRole + 1,
    NameRole,
    IsGroupRole,
    StatusRole,
    ErrorRole,
    SizeRole,
    LocalSizeRole,
    ProgressRole,
    MapsCountRole,
    LocalMapsCountRole,
    DescriptionRole,
    SectionRole,
    FoundNameRole,
    ParentNameRole,
    // The map file is downloaded, possibly outdated or being updated; false for groups.
    PresentRole,
    // World maps can't be deleted.
    DeletableRole,
  };

  explicit CountriesModel(QObject * parent = nullptr);
  ~CountriesModel() override;

  QString parentId() const;
  void setParentId(QString const & parentId);
  QString title() const;
  bool downloadedOnly() const { return m_downloadedOnly; }
  void setDownloadedOnly(bool downloadedOnly);
  QString query() const { return m_query; }
  void setQuery(QString const & query);

  int rowCount(QModelIndex const & parent = QModelIndex()) const override;
  QVariant data(QModelIndex const & index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void download(QString const & countryId);
  Q_INVOKABLE void cancel(QString const & countryId);
  Q_INVOKABLE void remove(QString const & countryId);
  Q_INVOKABLE void showOnMap(QString const & countryId);
  // Updates the outdated maps in the parent node.
  Q_INVOKABLE void updateAll();
  // Cancels the downloads in the parent node.
  Q_INVOKABLE void cancelAll();
  // Space for download(): its download, or its update when outdated.
  Q_INVOKABLE bool hasSpaceFor(QString const & countryId) const;
  Q_INVOKABLE bool hasSpaceToUpdate(QString const & countryId) const;
  Q_INVOKABLE bool hasUnsavedEdits(QString const & countryId) const;

  QString updateSize() const;
  int parentStatus() const;

signals:
  void parentIdChanged();
  void downloadedOnlyChanged();
  void queryChanged();
  void updatesChanged();

private:
  void Reload();
  void SetChildren(storage::CountriesVec && children);
  void OnCountryChanged(storage::CountryId const & countryId);
  void OnProgress(storage::CountryId const & countryId);

  storage::Storage & m_storage;
  storage::CountryId m_parentId;
  storage::CountriesVec m_children;
  // The first rows are "Near me" ones.
  size_t m_nearCount = 0;
  std::vector<std::string> m_foundNames;
  bool m_downloadedOnly = false;
  QString m_query;
  uint64_t m_searchTimestamp = 0;
  int m_storageSlot = 0;
};
}  // namespace sailfish
