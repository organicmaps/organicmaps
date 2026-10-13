#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

#include <cstdint>
#include <memory>
#include <string>

class Framework;

// Qt 5.6 moc can't parse the search headers, so the results are kept behind a pointer.
namespace search
{
class Results;
}  // namespace search

namespace sailfish
{
// Everywhere search for the list, plus a viewport search for the map marks.
class SearchModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
  Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
  // Newest first.
  Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)
  // Of the matched parts of names, which are styled text.
  Q_PROPERTY(QColor highlightColor MEMBER m_highlightColor)

public:
  enum Roles
  {
    NameRole = Qt::UserRole + 1,
    DescriptionRole,
    AddressRole,
    DistanceRole,
    OpenStatusRole,
    OpenStateRole,
    // A completion of the query rather than a place.
    SuggestRole
  };

  enum OpenState
  {
    OpenUnknown,
    Open,
    ClosingSoon,
    OpeningSoon,
    Closed
  };
  Q_ENUM(OpenState)

  explicit SearchModel(QObject * parent = nullptr);
  ~SearchModel() override;

  int rowCount(QModelIndex const & parent = {}) const override;
  QVariant data(QModelIndex const & index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  QString query() const { return m_query; }
  void setQuery(QString const & query);
  bool searching() const { return m_searching; }
  QStringList history() const;

  // As {key, name} in the search language.
  Q_INVOKABLE QVariantList categories() const;
  Q_INVOKABLE void searchCategory(QString const & name, bool addToHistory = true);
  // In the language of the link, if it has one, until the query changes.
  Q_INVOKABLE void searchFromLink(QString const & query, QString const & locale);
  // Returns false for a suggestion, which replaces the query instead of selecting a place.
  Q_INVOKABLE bool activate(int row);
  Q_INVOKABLE void showOnMap();
  Q_INVOKABLE void clearHistory();

signals:
  void queryChanged();
  void searchingChanged();
  void historyChanged();

private:
  void Run();
  void OnResults(uint64_t timestamp, search::Results && results);
  void SetSearching(bool searching);
  void SaveToHistory(QString const & query);

  Framework & m_framework;
  QString m_query;
  QString m_categoryQuery;
  std::string m_linkLocale;
  std::unique_ptr<search::Results> m_results;
  uint64_t m_timestamp = 0;
  // Waits for a pause in typing, as on Android.
  QTimer m_runTimer;
  bool m_searching = false;
  QColor m_highlightColor;
};
}  // namespace sailfish
