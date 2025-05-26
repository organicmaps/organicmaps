#include "qt/about.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <string>

#include <QtCore/QFile>
#include <QtGui/QIcon>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>

AboutDialog::AboutDialog(QWidget * parent)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
            Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint)
{
  QIcon icon(":/ui/logo.png");
  setWindowIcon(icon);
  setWindowTitle(QMenuBar::tr("About"));

  QLabel * labelIcon = new QLabel();
  labelIcon->setPixmap(icon.pixmap(128));

  Platform & platform = GetPlatform();

  QVBoxLayout * versionBox = new QVBoxLayout();
  versionBox->addWidget(new QLabel(QCoreApplication::applicationName()));
  QLabel * versionLabel = new QLabel("Version: " + QString::fromStdString(platform.Version()));
  versionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  versionBox->addWidget(versionLabel);
  // TODO: insert maps data version.
  //versionBox->addWidget(new QLabel(QString("Data: ") + DESIGNER_DATA_VERSION));

  QHBoxLayout * hBox = new QHBoxLayout();
  hBox->addWidget(labelIcon);
  hBox->addLayout(versionBox);

  std::string aboutText;
  try
  {
    ReaderPtr<Reader> reader = platform.GetReader("copyright.html");
    reader.ReadAsString(aboutText);
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("About text error: ", ex.Msg()));
  }

  if (!aboutText.empty())
  {
    QTextBrowser * aboutTextBrowser = new QTextBrowser();
    aboutTextBrowser->setReadOnly(true);
    aboutTextBrowser->setOpenLinks(true);
    aboutTextBrowser->setOpenExternalLinks(true);
    aboutTextBrowser->setText(aboutText.c_str());

    QVBoxLayout * vBox = new QVBoxLayout();
    vBox->addLayout(hBox);
    vBox->addWidget(aboutTextBrowser);
    setLayout(vBox);
  }
  else
    setLayout(hBox);

  adjustSize();
}
