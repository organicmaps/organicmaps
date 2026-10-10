#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <atomic>
#include <thread>

class Framework;

namespace sailfish
{
// Moving maps takes the whole writable folder (maps, edits, track); the app quits then and uses it on the next start.
class MapsStorage : public QObject
{
  Q_OBJECT
  // Locations as {path, name, details, current}.
  Q_PROPERTY(QVariantList locations READ locations NOTIFY changed)
  Q_PROPERTY(QString currentName READ currentName NOTIFY changed)
  Q_PROPERTY(bool moving READ moving NOTIFY changed)
  // The maps were moved: the app must quit before anything writes into the old folder.
  Q_PROPERTY(bool moved READ moved NOTIFY changed)
  Q_PROPERTY(QString downloadedSize READ downloadedSize NOTIFY changed)

public:
  explicit MapsStorage(Framework & framework, QObject * parent = nullptr);
  ~MapsStorage() override;

  // The chosen folder if it is still there; read before the platform starts.
  static QString ConfiguredDir();

  QVariantList locations() const;
  QString currentName() const;
  bool moving() const { return m_moving; }
  bool moved() const { return m_moved; }
  // Nothing may write into the folder while it moves or after it moved.
  bool Locked() const { return m_moving || m_moved; }
  // Locked() for the code without a MapsStorage, e.g. the map auto-download.
  static bool IsLocked() { return s_locked; }
  QString downloadedSize() const;

  // Not while a map downloads, a route is active or a track records: they write into the folder.
  Q_INVOKABLE bool canMove() const;
  // False without moving when !canMove(), e.g. a download started during the remorse timer; otherwise
  // moveFinished() follows.
  Q_INVOKABLE bool moveTo(QString const & path);
  // E.g. after a memory card was inserted.
  Q_INVOKABLE void refresh() { emit changed(); }

signals:
  void changed();
  // On success the app must restart. Fails without moving when a file of the same name is in the new folder:
  // it may be data left there while the memory card was out.
  void moveFinished(bool success);
  // From the moving thread.
  void treeMoved(QString const & path, bool success);

private:
  void OnMoved(QString const & path, bool success);

  Framework & m_framework;
  bool m_moving = false;
  bool m_moved = false;
  static std::atomic<bool> s_locked;
  // Set on exit, which mustn't wait for all the maps to copy.
  std::atomic<bool> m_cancelled = false;
  std::thread m_thread;
};
}  // namespace sailfish
