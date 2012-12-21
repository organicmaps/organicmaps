#include "about.hpp"

#include "../platform/platform.hpp"

#include <QtCore/QFile>

#include <QtGui/QIcon>

#include <QtWidgets/QMenuBar>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTextBrowser>



AboutDialog::AboutDialog(QWidget * parent)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
  QIcon icon(":/ui/logo.png");
  setWindowIcon(icon);
  setWindowTitle(QMenuBar::tr("About"));

  QLabel * labelIcon = new QLabel();
  labelIcon->setPixmap(icon.pixmap(128));

  // @todo insert version to bundle.
  QLabel * labelVersion = new QLabel(QString::fromLocal8Bit("MapsWithMe"));

  QHBoxLayout * hBox = new QHBoxLayout();
  hBox->addWidget(labelIcon);
  hBox->addWidget(labelVersion);

  string aboutText;
  try
  {
    ReaderPtr<Reader> reader = GetPlatform().GetReader("about.html");
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
