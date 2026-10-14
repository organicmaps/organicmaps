#include "sailfish/osm_account.hpp"

#include "sailfish/helpers.hpp"

#include "editor/osm_auth.hpp"
#include "editor/osm_editor.hpp"
#include "editor/server_api.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "base/logging.hpp"
#include "base/timer.hpp"

#include <QDesktopServices>
#include <QGuiApplication>
#include <QPointer>
#include <QUrl>

namespace sailfish
{
namespace
{
std::string_view constexpr kToken = "SailfishOsmToken";
std::string_view constexpr kUserName = "SailfishOsmUserName";
std::string_view constexpr kChangesets = "SailfishOsmChangesets";
std::string_view constexpr kImageUrl = "SailfishOsmImageUrl";

std::string Token()
{
  return LoadSetting(kToken, std::string());
}
}  // namespace

OsmAccount::OsmAccount(QObject * parent) : QObject(parent)
{
  updateEdits();
  // No background job: upload on start and when the app is put aside.
  connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state)
  {
    if (state != Qt::ApplicationActive)
      uploadChanges();
  });
  uploadChanges();
}

bool OsmAccount::loggedIn() const
{
  return !Token().empty();
}

QString OsmAccount::userName() const
{
  return QString::fromStdString(LoadSetting(kUserName, std::string()));
}

int OsmAccount::changesets() const
{
  return LoadSetting(kChangesets, -1);
}

QString OsmAccount::historyUrl() const
{
  return QString::fromStdString(osm::OsmOAuth::ServerAuth().GetHistoryURL(userName().toStdString()));
}

QString OsmAccount::notesUrl() const
{
  return QString::fromStdString(osm::OsmOAuth::ServerAuth().GetNotesURL(userName().toStdString()));
}

QString OsmAccount::imageUrl() const
{
  return QString::fromStdString(LoadSetting(kImageUrl, std::string()));
}

QString OsmAccount::registrationUrl() const
{
  return QString::fromStdString(osm::OsmOAuth::ServerAuth().GetRegistrationURL());
}

void OsmAccount::loginInBrowser()
{
  m_browserLoginStarted = true;
  QDesktopServices::openUrl(QUrl(QString::fromStdString(osm::OsmOAuth::ServerAuth().BuildOAuth2Url())));
}

void OsmAccount::LoginWithCode(QString const & code)
{
  // Any app can send an om:// link: take a code only for a login the user started, once.
  if (!m_browserLoginStarted)
  {
    LOG(LWARNING, ("Ignored an OAuth2 code without a login in the browser"));
    return;
  }
  m_browserLoginStarted = false;
  if (m_loggingIn)
    return;
  m_loggingIn = true;
  emit busyChanged();

  QPointer<OsmAccount> self(this);
  GetPlatform().RunTask(Platform::Thread::Network, [self, code = code.toStdString()]
  {
    std::string token;
    try
    {
      token = osm::OsmOAuth::ServerAuth().FinishAuthorization(code);
    }
    catch (std::exception const & e)
    {
      LOG(LWARNING, ("OSM login failed:", e.what()));
    }

    GetPlatform().RunTask(Platform::Thread::Gui, [self, token]
    {
      if (!self)
        return;
      self->m_loggingIn = false;
      emit self->busyChanged();
      if (token.empty())
      {
        emit self->loginFailed(Localized("editor_login_error_dialog"));
        return;
      }
      settings::Set(kToken, token);
      emit self->changed();
      self->LoadProfile();
      self->uploadChanges();
    });
  });
}

void OsmAccount::logout()
{
  settings::Delete(kToken);
  settings::Delete(kUserName);
  settings::Delete(kChangesets);
  settings::Delete(kImageUrl);
  emit changed();
}

void OsmAccount::LoadProfile()
{
  QPointer<OsmAccount> self(this);
  GetPlatform().RunTask(Platform::Thread::Network, [self, token = Token()]
  {
    osm::UserPreferences prefs;
    try
    {
      prefs = osm::ServerApi06(osm::OsmOAuth::ServerAuth(token)).GetUserPreferences();
    }
    catch (std::exception const & e)
    {
      LOG(LWARNING, ("Can't load OSM user preferences:", e.what()));
      return;
    }

    GetPlatform().RunTask(Platform::Thread::Gui, [self, prefs]
    {
      // Logged out meanwhile.
      if (!self || !self->loggedIn())
        return;
      settings::Set(kUserName, prefs.m_displayName);
      settings::Set(kChangesets, static_cast<int>(prefs.m_changesets));
      settings::Set(kImageUrl, prefs.m_imageUrl);
      emit self->changed();
    });
  });
}

void OsmAccount::uploadChanges()
{
  // A new edit or note counts even when it can't go up yet.
  updateEdits();
  auto const token = Token();
  if (token.empty() || m_uploading)
    return;

  QPointer<OsmAccount> self(this);
  auto const onFinish = [self](osm::Editor::UploadResult result)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [self, result]
    {
      if (!self)
        return;
      self->m_uploading = false;
      emit self->busyChanged();
      self->updateEdits();
      if (result == osm::Editor::UploadResult::Success)
        self->LoadProfile();
    });
  };

  auto const started = osm::Editor::Instance().UploadChanges(
      token, {{"created_by", "Organic Maps Sailfish " + GetPlatform().Version()}}, onFinish);
  if (started == osm::Editor::UploadStart::Started)
  {
    m_uploading = true;
    emit busyChanged();
  }
}

void OsmAccount::updateEdits()
{
  auto const stats = osm::Editor::Instance().GetStats();
  m_pendingEdits = static_cast<int>(stats.m_edits.size() - stats.m_uploadedCount);
  m_lastUpload = stats.m_lastUploadTimestamp == base::INVALID_TIME_STAMP
                   ? QDateTime()
                   : QDateTime::fromTime_t(static_cast<uint>(stats.m_lastUploadTimestamp));
  emit editsChanged();
}
}  // namespace sailfish
