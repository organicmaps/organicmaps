#include "about.hpp"

#include "../version/version.hpp"

#include "../platform/platform.hpp"

#include <QtCore/QFile>
#include <QtGui/QIcon>
#include <QtGui/QMenuBar>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>

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

  QFile file(GetPlatform().ReadPathForFile("about.html").c_str());
  if (file.open(QIODevice::ReadOnly))
  {
    QByteArray aboutData = file.readAll();
    file.close();

    QLabel * labelAbout = new QLabel(aboutData);
    labelAbout->setOpenExternalLinks(true);

    QVBoxLayout * vBox = new QVBoxLayout();
    vBox->addLayout(hBox);
    vBox->addWidget(labelAbout);
    setLayout(vBox);
  }
  else
    setLayout(hBox);
}
