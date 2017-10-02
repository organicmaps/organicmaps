#include "openlr/openlr_match_quality/openlr_assessment_tool/web_view.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <QContextMenuEvent>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>

namespace
{
bool IsLoginUrl(QString const & url)
{
  return url.contains("Login");
}
}  // namespace

namespace openlr
{
WebView::WebView(std::string const & url, std::string const & login, std::string const & password)
  : QWebEngineView(nullptr), m_loadProgress(0), m_url(url), m_login(login), m_password(password)
{
  connect(this, &QWebEngineView::loadProgress, [this](int progress) { m_loadProgress = progress; });
  connect(this, &QWebEngineView::loadFinished, [this](bool success) {
    if (!success)
    {
      m_loadProgress = 0;
    }
  });

  connect(this, &QWebEngineView::renderProcessTerminated,
          [this](QWebEnginePage::RenderProcessTerminationStatus termStatus, int statusCode) {
            QString status;
            switch (termStatus)
            {
            case QWebEnginePage::NormalTerminationStatus:
              status = "Render process normal exit";
              break;
            case QWebEnginePage::AbnormalTerminationStatus:
              status = "Render process abnormal exit";
              break;
            case QWebEnginePage::CrashedTerminationStatus:
              status = "Render process crashed";
              break;
            case QWebEnginePage::KilledTerminationStatus:
              status = "Render process killed";
              break;
            }
            QMessageBox::StandardButton btn =
                QMessageBox::question(window(), status,
                                      tr("Render process exited with code: %1\n"
                                         "Do you want to reload the page ?")
                                          .arg(statusCode));
            if (btn == QMessageBox::Yes)
              QTimer::singleShot(0, [this] { reload(); });
          });

  connect(this, &QWebEngineView::loadFinished, this, &WebView::OnLoadFinished);
  load(QString(m_url.data()));
}

int WebView::loadProgress() const { return m_loadProgress; }

void WebView::SetCurrentSegment(int segmentId)
{
  m_requestedSegmentId = segmentId;
  LOG(LDEBUG, ("SegmentID seg to", segmentId));
  if (!IsLoginUrl(url().toString()))
    GoToSegment();
}

bool WebView::isWebActionEnabled(QWebEnginePage::WebAction webAction) const
{
  return page()->action(webAction)->isEnabled();
}

QWebEngineView * WebView::createWindow(QWebEnginePage::WebWindowType type)
{
  CHECK(false, ("We are not going to use but one window."));
  return nullptr;
}

void WebView::contextMenuEvent(QContextMenuEvent * event)
{
  CHECK(false, ("No context menu is necessary."));
}

void WebView::OnLoadFinished(bool ok)
{
  if (!ok)
  {
    LOG(LDEBUG, ("Page is not loaded properly, giving up"));
    return;
  }

  if (IsLoginUrl(url().toString()))
    Login();
  else
    GoToSegment();
}

void WebView::GoToSegment()
{
  CHECK(!IsLoginUrl(url().toString()), ("Can't go to a segment on the login page."));
  if (m_requestedSegmentId == kInvalidId)
    return;

  auto const script = QString(R"EOT(
    function s(ctx, arg) { return ctx.querySelector(arg); }
    function uncheck(cb) { if (cb.checked) cb.click(); }
    uncheck(s(document, "#inrix\\:filters\\:traffic\\:trafficflowvisible"));
    uncheck(s(document, "#inrix\\:filters\\:traffic\\:incidentsvisible"));
    var navSpan = s(document, "#inrix\\:navigation\\:contextual");
    var input = s(navSpan, "input.FreeFormInput");
    input.value = %1;
    input.classList.remove("WaterMark");
    s(navSpan, "button.LocationSearchButton").click();
  )EOT").arg(m_requestedSegmentId);

  page()->runJavaScript(script, [](QVariant const & v) {
    LOG(LDEBUG, ("DEMO JS is done:", v.toString().toStdString()));
  });
  m_requestedSegmentId = kInvalidId;
}

void WebView::Login()
{
  auto const script = QString(R"EOT(
    function select(arg) { return document.querySelector(arg); }
    select("#ctl00_BodyPlaceHolder_LoginControl_UserName").value = "%1";
    select("#ctl00_BodyPlaceHolder_LoginControl_Password").value = "%2";
    select("#ctl00_BodyPlaceHolder_LoginControl_LoginButton").click();
  )EOT").arg(m_login.data()).arg(m_password.data());

  page()->runJavaScript(script, [](QVariant const & v) {
    LOG(LDEBUG, ("Login JS is done:", v.toString().toStdString()));
  });
}
}  // namespace openlr
