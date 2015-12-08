#include "qt/about.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <QtCore/QFile>
#include <QtGui/QIcon>


#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QMenuBar>
  #include <QtGui/QHBoxLayout>
  #include <QtGui/QVBoxLayout>
  #include <QtGui/QLabel>
  #include <QtGui/QTextBrowser>
#else
  #include <QtWidgets/QMenuBar>
  #include <QtWidgets/QHBoxLayout>
  #include <QtWidgets/QVBoxLayout>
  #include <QtWidgets/QLabel>
  #include <QtWidgets/QTextBrowser>
#endif

#ifdef BUILD_DESIGNER
  #include "designer_version.h"
#endif // BUILD_DESIGNER

AboutDialog::AboutDialog(QWidget * parent)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
  QIcon icon(":/ui/logo.png");
  setWindowIcon(icon);
  setWindowTitle(QMenuBar::tr("About"));

  QLabel * labelIcon = new QLabel();
  labelIcon->setPixmap(icon.pixmap(128));

#ifndef BUILD_DESIGNER
  // @todo insert version to bundle.
  QLabel * labelVersion = new QLabel(qAppName());

  QHBoxLayout * hBox = new QHBoxLayout();
  hBox->addWidget(labelIcon);
  hBox->addWidget(labelVersion);
#else // BUILD_DESIGNER
  QVBoxLayout * versionBox = new QVBoxLayout();
  versionBox->addWidget(new QLabel(qAppName()));
  versionBox->addWidget(new QLabel(QString("Version: ") + DESIGNER_APP_VERSION));
  versionBox->addWidget(new QLabel(QString("Commit: ") + DESIGNER_CODEBASE_SHA));
  versionBox->addWidget(new QLabel(QString("Data: ") + DESIGNER_DATA_VERSION));

  QHBoxLayout * hBox = new QHBoxLayout();
  hBox->addWidget(labelIcon);
  hBox->addLayout(versionBox);
#endif

  string aboutText;
  try
  {
    ReaderPtr<Reader> reader = GetPlatform().GetReader("copyright.html");
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
}
