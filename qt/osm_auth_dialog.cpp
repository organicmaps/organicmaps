#include "qt/osm_auth_dialog.hpp"

#include "editor/osm_auth.hpp"

#include "platform/settings.hpp"

#include <string>

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

using namespace osm;

namespace qt
{
char const * kTokenKeySetting = "OsmTokenKey";
char const * kTokenSecretSetting = "OsmTokenSecret";
char const * kLoginDialogTitle = "OpenStreetMap Login";
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
  bool const isLoginDialog = findChild<QPushButton *>("button")->text() == tr("Login");
  if (isLoginDialog)
  {
    std::string const login = findChild<QLineEdit *>("login")->text().toStdString();
    std::string const password = findChild<QLineEdit *>("password")->text().toStdString();

    if (login.empty())
    {
      setWindowTitle("Please enter Login");
      return;
    }

    if (password.empty())
    {
      setWindowTitle("Please enter Password");
      return;
    }

    OsmOAuth auth = osm::OsmOAuth::ServerAuth();
    try
    {
      if (auth.AuthorizePassword(login, password))
      {
        auto const token = auth.GetAuthToken();
        settings::Set(kOauthTokenSetting, token);

        SwitchToLogout(this);
      }
      else
      {
        setWindowTitle("Auth failed: invalid login or password");
        return;
      }
    }
    catch (std::exception const & ex)
    {
      setWindowTitle((std::string("Auth failed: ") + ex.what()).c_str());
      return;
    }
  }
  else
  {
    settings::Set(kOauthTokenSetting, std::string());

    SwitchToLogin(this);
  }
}
}  // namespace qt
