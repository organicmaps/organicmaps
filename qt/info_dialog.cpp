#include "qt/info_dialog.hpp"

#include "base/assert.hpp"

#include <QtGui/QIcon>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>

namespace qt
{
  InfoDialog::InfoDialog(QString const & title, QString const & text, QWidget * parent,
                         QStringList const & buttons)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
  {
    QIcon icon(":/ui/logo.png");
    setWindowIcon(icon);
    setWindowTitle(title);
    setFocusPolicy(Qt::StrongFocus);
    setWindowModality(Qt::WindowModal);

    QVBoxLayout * vBox = new QVBoxLayout();
    QTextBrowser * browser = new QTextBrowser();
    browser->setReadOnly(true);
    browser->setOpenLinks(true);
    browser->setOpenExternalLinks(true);
    browser->setText(text);
    vBox->addWidget(browser);

    // this horizontal layout is for buttons
    QHBoxLayout * hBox = new QHBoxLayout();
    hBox->addSpacing(browser->width() / 4 * (3.5 - buttons.size()));
    for (int i = 0; i < buttons.size(); ++i)
    {
      QPushButton * button = new QPushButton(buttons[i], this);
      switch (i)
      {
      case 0: connect(button, SIGNAL(clicked()), this, SLOT(OnButtonClick1())); break;
      case 1: connect(button, SIGNAL(clicked()), this, SLOT(OnButtonClick2())); break;
      case 2: connect(button, SIGNAL(clicked()), this, SLOT(OnButtonClick3())); break;
      default:
        ASSERT(false, ("Only 3 buttons are currently supported in info dialog"));
      }
      hBox->addWidget(button);
    }

    vBox->addLayout(hBox);
    setLayout(vBox);
  }

  void InfoDialog::OnButtonClick1()
  {
    done(1);
  }
  void InfoDialog::OnButtonClick2()
  {
    done(2);
  }
  void InfoDialog::OnButtonClick3()
  {
    done(3);
  }
}
