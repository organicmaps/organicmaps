#include "qt/osm_auth_dialog.hpp"

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
char const * kLoginDialogTitle = "OpenStreetMap Login";
char const * kOsmLoginSetting = "OsmLogin";
char const * kOauthTokenSetting = "OsmOauthToken";

OsmAuthDialog::OsmAuthDialog(QWidget * parent)
{
  std::string token;
  bool const isLoginDialog = !settings::Get(kOauthTokenSetting, token) || token.empty();

  QVBoxLayout * vLayout = new QVBoxLayout(parent);

  QHBoxLayout * loginRow = new QHBoxLayout();
  loginRow->addWidget(new QLabel(tr("Username/email:")));
  QLineEdit * loginLineEdit = new QLineEdit();
  loginLineEdit->setObjectName("login");
  if (!isLoginDialog)
    loginLineEdit->setEnabled(false);
  if (std::string login; settings::Get(kOsmLoginSetting, login))
    loginLineEdit->setText(login.c_str());
  loginRow->addWidget(loginLineEdit);
  vLayout->addLayout(loginRow);

  QHBoxLayout * passwordRow = new QHBoxLayout();
  passwordRow->addWidget(new QLabel(tr("Password:")));
  QLineEdit * passwordLineEdit = new QLineEdit();
  passwordLineEdit->setEchoMode(QLineEdit::Password);
  passwordLineEdit->setObjectName("password");
  if (!isLoginDialog)
    passwordLineEdit->setEnabled(false);
  passwordRow->addWidget(passwordLineEdit);
  vLayout->addLayout(passwordRow);

  QPushButton * actionButton = new QPushButton(isLoginDialog ? tr("Login") : tr("Logout"));
  actionButton->setObjectName("button");
  actionButton->setDefault(true);
  connect(actionButton, &QAbstractButton::clicked, this, &OsmAuthDialog::OnAction);

  QPushButton * closeButton = new QPushButton(tr("Close"));
  connect(closeButton, &QAbstractButton::clicked, this, &QWidget::close);

  QHBoxLayout * buttonsLayout = new QHBoxLayout();
  buttonsLayout->addWidget(closeButton);
  buttonsLayout->addWidget(actionButton);
  vLayout->addLayout(buttonsLayout);

  setLayout(vLayout);
  setWindowTitle(tr(kLoginDialogTitle));
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
  dlg->findChild<QPushButton *>("button")->setText(dlg->tr("Login"));
}

void SwitchToLogout(QDialog * dlg)
{
  dlg->findChild<QLineEdit *>("login")->setEnabled(false);
  dlg->findChild<QLineEdit *>("password")->setEnabled(false);
  dlg->findChild<QPushButton *>("button")->setText(dlg->tr("Logout"));
  dlg->setWindowTitle(dlg->tr(kLoginDialogTitle));
}

void OsmAuthDialog::OnAction()
{
  auto actionButton = findChild<QPushButton *>("button");
  bool const isLoginDialog = actionButton->text() == tr("Login");
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
    setWindowTitle("Please enter Login");
    return;
  }

  settings::Set(kOsmLoginSetting, login);

  if (password.empty())
  {
    setWindowTitle("Please enter Password");
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
        runOnMainThread = [this]() { setWindowTitle("Auth failed: invalid login or password"); };
    }
    catch (std::exception const & ex)
    {
      auto const title = std::string("Auth failed: ") + ex.what();
      runOnMainThread = [this, title]() { setWindowTitle(title.c_str()); };
    }
    QMetaObject::invokeMethod(this, [actionButton, runOnMainThread = std::move(runOnMainThread)]()
    {
      actionButton->setEnabled(true);
      runOnMainThread();
    }, Qt::QueuedConnection);
  });
}
}  // namespace qt
