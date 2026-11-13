#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <functional>

class Framework;

namespace sailfish
{
// Moving maps takes the whole writable folder (maps, edits, track). Nothing may write into it while it moves,
// so the move is only recorded here and done on the next start, before the platform and the core use the folder.
class MapsStorage : public QObject
{
  Q_OBJECT
  // Locations as {path, name, details, current}.
  Q_PROPERTY(QVariantList locations READ locations NOTIFY changed)
  Q_PROPERTY(QString currentName READ currentName NOTIFY changed)
  // A move waits for the app to start again.
  Q_PROPERTY(bool movePending READ movePending NOTIFY changed)
  // The move at this start failed and the maps stayed where they were.
  Q_PROPERTY(bool moveFailed READ moveFailed CONSTANT)
  Q_PROPERTY(QString downloadedSize READ downloadedSize NOTIFY changed)

public:
  explicit MapsStorage(Framework & framework, QObject * parent = nullptr);

  static bool HasPendingMove();
  // Does the pending move, reporting the copied part (0 to 1), then returns the folder to use; empty for the default
  // one. Before the platform starts.
  static QString ApplyPendingMove(std::function<void(double)> const & onProgress);

  QVariantList locations() const;
  QString currentName() const;
  bool movePending() const { return m_movePending; }
  bool moveFailed() const { return s_moveFailed; }
  QString downloadedSize() const;

  // The move happens on the next start. False if it can't be saved.
  Q_INVOKABLE bool moveTo(QString const & path);
  // E.g. after a memory card was inserted.
  Q_INVOKABLE void refresh() { emit changed(); }

signals:
  void changed();

private:
  Framework & m_framework;
  bool m_movePending = false;
  static bool s_moveFailed;
};
}  // namespace sailfish
