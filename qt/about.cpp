#include "about.hpp"

#include "../version/version.hpp"

#include "../platform/platform.hpp"

#include <QtCore/QFile>
#include <QtGui/QIcon>
#include <QtGui/QMenuBar>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QTextBrowser>

AboutDialog::AboutDialog(QWidget * parent)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
  QIcon icon(":logo.png");
  setWindowIcon(icon);
  setWindowTitle(QMenuBar::tr("About"));

  QLabel * labelIcon = new QLabel();
  labelIcon->setPixmap(icon.pixmap(128));

  QString const versionString = tr("<h3>%1 %2</h3>"
                        "<br/>"
                        "Built on %3"
                        ).arg(QString("MapsWithMe"), QLatin1String(VERSION_STRING),
                              QLatin1String(VERSION_DATE_STRING));
  QLabel * labelVersion = new QLabel(versionString);

  QHBoxLayout * hBox = new QHBoxLayout();
  hBox->addWidget(labelIcon);
  hBox->addWidget(labelVersion);

  char const sAboutFileName [] = "about-travelguide-desktop.html";
  QFile file(GetPlatform().ReadPathForFile(sAboutFileName).c_str());
  if (file.open(QIODevice::ReadOnly))
  {
    QByteArray aboutData = file.readAll();
    file.close();

    QTextBrowser * aboutTextBrowser = new QTextBrowser();
    aboutTextBrowser->setReadOnly(true);
    aboutTextBrowser->setOpenLinks(true);
    aboutTextBrowser->setOpenExternalLinks(true);
    aboutTextBrowser->setText(aboutData.constData());

    QVBoxLayout * vBox = new QVBoxLayout();
    vBox->addLayout(hBox);
    vBox->addWidget(aboutTextBrowser);
    setLayout(vBox);
  }
  else
    setLayout(hBox);
}
