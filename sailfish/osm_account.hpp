#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>

namespace sailfish
{
// Edits are kept locally and uploaded once logged in.
class OsmAccount : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY changed)
  Q_PROPERTY(QString userName READ userName NOTIFY changed)
  // -1 until loaded.
  Q_PROPERTY(int changesets READ changesets NOTIFY changed)
  Q_PROPERTY(QString historyUrl READ historyUrl NOTIFY changed)
  Q_PROPERTY(QString notesUrl READ notesUrl NOTIFY changed)
  Q_PROPERTY(QString imageUrl READ imageUrl NOTIFY changed)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(int pendingEdits READ pendingEdits NOTIFY editsChanged)
  // Invalid before the first upload.
  Q_PROPERTY(QDateTime lastUpload READ lastUpload NOTIFY editsChanged)
  Q_PROPERTY(QString registrationUrl READ registrationUrl CONSTANT)

public:
  explicit OsmAccount(QObject * parent = nullptr);

  bool loggedIn() const;
  QString userName() const;
  int changesets() const;
  QString historyUrl() const;
  QString notesUrl() const;
  QString imageUrl() const;
  bool busy() const { return m_loggingIn || m_uploading; }
  int pendingEdits() const { return m_pendingEdits; }
  QDateTime lastUpload() const { return m_lastUpload; }
  QString registrationUrl() const;

  // The browser returns the OAuth2 code by an om:// link, for LoginWithCode().
  Q_INVOKABLE void loginInBrowser();
  // Emits changed or loginFailed when done.
  void LoginWithCode(QString const & code);
  Q_INVOKABLE void logout();
  // Recounts the pending edits, and uploads them and the notes when logged in.
  Q_INVOKABLE void uploadChanges();
  Q_INVOKABLE void updateEdits();

signals:
  void changed();
  void busyChanged();
  void editsChanged();
  void loginFailed(QString const & message);

private:
  void LoadProfile();

  bool m_loggingIn = false;
  bool m_browserLoginStarted = false;
  bool m_uploading = false;
  int m_pendingEdits = 0;
  QDateTime m_lastUpload;
};
}  // namespace sailfish
