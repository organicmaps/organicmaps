#pragma once

#include <QObject>
#include <QString>

#include <cstdint>

class Framework;

namespace sailfish
{
class BookmarkEditor : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool isTrack READ isTrack NOTIFY loaded)
  Q_PROPERTY(QString name READ name NOTIFY loaded)
  Q_PROPERTY(QString description READ description NOTIFY loaded)
  // Index in appInfo.bookmarkColors, or -1 for a custom color, which is kept unless another one is chosen.
  Q_PROPERTY(int colorIndex READ colorIndex NOTIFY loaded)
  Q_PROPERTY(quint64 listId READ listId NOTIFY loaded)
  Q_PROPERTY(bool trackVisible READ trackVisible NOTIFY loaded)

public:
  explicit BookmarkEditor(QObject * parent = nullptr);

  bool trackVisible() const;
  Q_INVOKABLE void loadBookmark(quint64 id);
  Q_INVOKABLE void loadTrack(quint64 id);
  // trackVisible applies to a track only.
  Q_INVOKABLE void save(QString const & name, QString const & description, int colorIndex, quint64 listId,
                        bool trackVisible);

  bool isTrack() const { return m_isTrack; }
  QString name() const { return m_name; }
  QString description() const { return m_description; }
  int colorIndex() const { return m_colorIndex; }
  quint64 listId() const { return m_listId; }

signals:
  void loaded();

private:
  bool IsSelected() const;

  Framework & m_framework;
  bool m_isTrack = false;
  uint64_t m_id = 0;
  uint64_t m_listId = 0;
  QString m_name;
  QString m_description;
  int m_colorIndex = -1;
};
}  // namespace sailfish
