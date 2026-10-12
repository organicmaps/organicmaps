#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantList>

#include <cstdint>
#include <vector>

class Framework;

namespace sailfish
{
// Forwards BookmarkManager callbacks to every bookmarks model.
class BookmarksNotifier : public QObject
{
  Q_OBJECT

public:
  static BookmarksNotifier & Instance();
  static void LoadBookmarks(Framework & framework);

signals:
  void changed();
  void loaded();
  void fileLoaded(QString const & path, bool success);
  // The first fix, or the first after the position was lost: sorting by distance becomes available.
  void positionFound();
};

class BookmarkCategoriesModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(bool allInvisible READ allInvisible NOTIFY visibilityChanged)
  Q_PROPERTY(int recentlyDeletedCount READ recentlyDeletedCount NOTIFY recentlyDeletedChanged)

public:
  enum Roles
  {
    IdRole = Qt::UserRole + 1,
    NameRole,
    BookmarksCountRole,
    TracksCountRole,
    VisibleRole
  };

  explicit BookmarkCategoriesModel(QObject * parent = nullptr);

  int rowCount(QModelIndex const & parent = {}) const override;
  QVariant data(QModelIndex const & index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  bool allInvisible() const;
  int recentlyDeletedCount() const;

  Q_INVOKABLE void setVisible(int row, bool visible);
  Q_INVOKABLE void setAllVisible(bool visible);
  // By id: a remorse timer deletes it later, when the rows may have changed.
  Q_INVOKABLE void deleteList(quint64 listId);
  Q_INVOKABLE void showOnMap(int row);
  // {name, path, date} rows.
  Q_INVOKABLE QVariantList recentlyDeleted() const;
  Q_INVOKABLE void recoverDeleted(QStringList const & paths);
  Q_INVOKABLE void deleteForever(QStringList const & paths);

signals:
  void visibilityChanged();
  void recentlyDeletedChanged();

private:
  void Reset();

  Framework & m_framework;
  std::vector<uint64_t> m_ids;
};

class BookmarksModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(quint64 listId READ listId WRITE setListId NOTIFY listIdChanged)
  Q_PROPERTY(QString name READ name NOTIFY listInfoChanged)
  Q_PROPERTY(QString description READ description NOTIFY listInfoChanged)
  // The annotation, or else the description, as StyledText.
  Q_PROPERTY(QString descriptionText READ descriptionText NOTIFY listInfoChanged)
  // Of the whole list, whatever the search shows.
  Q_PROPERTY(int bookmarksCount READ bookmarksCount NOTIFY listInfoChanged)
  Q_PROPERTY(int tracksCount READ tracksCount NOTIFY listInfoChanged)
  // A SortingType, or -1 for the default order: tracks, then bookmarks.
  Q_PROPERTY(int sortingType READ sortingType WRITE setSortingType NOTIFY sortingTypeChanged)
  Q_PROPERTY(QVariantList sortingTypes READ sortingTypes NOTIFY listInfoChanged)
  Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
  enum Roles
  {
    IdRole = Qt::UserRole + 1,
    IsTrackRole,
    NameRole,
    // The feature type of a bookmark, the length of a track.
    TypeRole,
    // The section, like "Tracks" or "A week ago".
    BlockRole,
    ColorRole,
    // "" without a position.
    DistanceRole,
    VisibleRole
  };

  // Mirrors BookmarkManager::SortingType.
  enum SortingType
  {
    ByType,
    ByDistance,
    ByTime,
    ByName
  };
  Q_ENUM(SortingType)

  explicit BookmarksModel(QObject * parent = nullptr);

  int rowCount(QModelIndex const & parent = {}) const override;
  QVariant data(QModelIndex const & index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  quint64 listId() const { return m_listId; }
  void setListId(quint64 id);
  QString name() const;
  QString description() const;
  int sortingType() const { return m_sortingType; }
  void setSortingType(int type);
  QVariantList sortingTypes() const;
  QString descriptionText() const;
  int bookmarksCount() const;
  int tracksCount() const;
  // A search in the list, which shows matching bookmarks only.
  QString filter() const { return m_filter; }
  void setFilter(QString const & filter);

  Q_INVOKABLE void showOnMap(int row);
  // As {id, isTrack}, for actions that run after the rows may have changed, like a remorse delete with
  // BookmarksIO::deleteItems().
  Q_INVOKABLE QVariantList items(QVariantList const & rows) const;
  Q_INVOKABLE QString shareText(int row) const;
  Q_INVOKABLE void setListInfo(QString const & name, QString const & description);
  Q_INVOKABLE void setTrackVisible(int row, bool visible);
  Q_INVOKABLE void moveItems(QVariantList const & items, quint64 listId);
  Q_INVOKABLE void setItemsColor(QVariantList const & items, int colorIndex);
  Q_INVOKABLE void setAllColor(bool tracks, int colorIndex);
  Q_INVOKABLE void showListOnMap();
  Q_INVOKABLE void deleteList();

signals:
  void listIdChanged();
  void listInfoChanged();
  void sortingTypeChanged();
  void filterChanged();

private:
  struct Item
  {
    uint64_t m_id;
    bool m_isTrack;
    QString m_block;
  };

  void Reset();
  void Search(int request);
  void SetDefaultOrder();
  void SetItems(std::vector<Item> && items);
  // Null for a stale row.
  Item const * ItemAt(int row) const;
  QString ItemName(Item const & item) const;
  void SetColor(std::vector<uint64_t> const & marks, std::vector<uint64_t> const & tracks, int colorIndex);

  Framework & m_framework;
  quint64 m_listId = 0;
  int m_sortingType = -1;
  QString m_filter;
  // Sorting and search run in the background; results of an older request are dropped.
  int m_request = 0;
  std::vector<Item> m_items;
};
}  // namespace sailfish
