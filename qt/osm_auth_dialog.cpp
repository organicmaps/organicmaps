#include "qt/osm_auth_dialog.hpp"
#include "qt/qt_common/translations.hpp"

#include "editor/osm_auth.hpp"

#include "platform/settings.hpp"

#include <string>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace qt
{
char const * kTokenKeySetting = "OsmTokenKey";
char const * kTokenSecretSetting = "OsmTokenSecret";
char const * kOsmLoginSetting = "OsmLogin";
char const * kOauthTokenSetting = "OsmOauthToken";

OsmAuthDialog::OsmAuthDialog(QWidget * parent)
{
  std::string token;
  bool const isLoginDialog = !settings::Get(kOauthTokenSetting, token) || token.empty();

  QVBoxLayout * vLayout = new QVBoxLayout(parent);

  QHBoxLayout * loginRow = new QHBoxLayout();
  loginRow->addWidget(new QLabel(Tr("email_or_username") + ":"));
  QLineEdit * loginLineEdit = new QLineEdit();
  loginLineEdit->setObjectName("login");
  if (!isLoginDialog)
    loginLineEdit->setEnabled(false);
  if (std::string login; settings::Get(kOsmLoginSetting, login))
    loginLineEdit->setText(login.c_str());
  loginRow->addWidget(loginLineEdit);
  vLayout->addLayout(loginRow);

  QHBoxLayout * passwordRow = new QHBoxLayout();
  passwordRow->addWidget(new QLabel(Tr("password") + ":"));
  QLineEdit * passwordLineEdit = new QLineEdit();
  passwordLineEdit->setEchoMode(QLineEdit::Password);
  passwordLineEdit->setObjectName("password");
  if (!isLoginDialog)
    passwordLineEdit->setEnabled(false);
  passwordRow->addWidget(passwordLineEdit);
  vLayout->addLayout(passwordRow);

  QPushButton * actionButton = new QPushButton(isLoginDialog ? Tr("login") : Tr("logout"));
  actionButton->setObjectName("button");
  actionButton->setDefault(true);
  connect(actionButton, &QAbstractButton::clicked, this, &OsmAuthDialog::OnAction);

  QPushButton * closeButton = new QPushButton(Tr("close"));
  connect(closeButton, &QAbstractButton::clicked, this, &QWidget::close);

  QHBoxLayout * buttonsLayout = new QHBoxLayout();
  buttonsLayout->addWidget(closeButton);
  buttonsLayout->addWidget(actionButton);
  vLayout->addLayout(buttonsLayout);

  setLayout(vLayout);
  setWindowTitle(Tr("login_osm"));
}

OsmAuthDialog::~OsmAuthDialog()
{
  if (m_authThread.joinable())
    m_authThread.join();
}

void SwitchToLogin(QDialog * dlg)
{
  dlg->findChild<QLineEdit *>("login")->setEnabled(true);
  dlg->findChild<QLineEdit *>("password")->setEnabled(true);
  dlg->findChild<QPushButton *>("button")->setText(Tr("login"));
}

void SwitchToLogout(QDialog * dlg)
{
  dlg->findChild<QLineEdit *>("login")->setEnabled(false);
  dlg->findChild<QLineEdit *>("password")->setEnabled(false);
  dlg->findChild<QPushButton *>("button")->setText(Tr("logout"));
  dlg->setWindowTitle(Tr("login_osm"));
}

void OsmAuthDialog::OnAction()
{
  auto actionButton = findChild<QPushButton *>("button");
  bool const isLoginDialog = actionButton->text() == Tr("login");
  if (!isLoginDialog)
  {
    settings::Set(kOauthTokenSetting, std::string());
    SwitchToLogin(this);
    return;
  }

  std::string const login = findChild<QLineEdit *>("login")->text().toStdString();
  std::string const password = findChild<QLineEdit *>("password")->text().toStdString();

  if (login.empty())
  {
    setWindowTitle(Tr("desktop_enter_login"));
    return;
  }

  settings::Set(kOsmLoginSetting, login);

  if (password.empty())
  {
    setWindowTitle(Tr("desktop_enter_password"));
    return;
  }

  // Avoid running several logins in parallel.
  actionButton->setEnabled(false);

  // Clean up previous instance.
  if (m_authThread.joinable())
    m_authThread.join();
  // Main thread is blocked in dialog and hangs the UI when calling synchronous HTTP request without additional thread.
  m_authThread = std::thread([this, actionButton, login, password]
  {
    auto auth = osm::OsmOAuth::ServerAuth();
    std::function<void()> runOnMainThread;
    try
    {
      if (auth.AuthorizePassword(login, password))
      {
        auto const token = auth.GetAuthToken();
        settings::Set(kOauthTokenSetting, token);
        runOnMainThread = [this]() { SwitchToLogout(this); };
      }
      else
        runOnMainThread = [this]() { setWindowTitle(Tr("desktop_auth_failed_invalid_login_or_password")); };
    }
    catch (std::exception const & ex)
    {
      auto const title = Tr("desktop_auth_failed").arg(ex.what());
      runOnMainThread = [this, title]() { setWindowTitle(title); };
    }
    QMetaObject::invokeMethod(this, [actionButton, runOnMainThread = std::move(runOnMainThread)]()
    {
      actionButton->setEnabled(true);
      runOnMainThread();
    }, Qt::QueuedConnection);
  });
}
}  // namespace qt
