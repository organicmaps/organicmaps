#include "info_dialog.hpp"

#include <QtGui/QIcon>
#include <QtGui/QTextEdit>
#include <QtGui/QPushButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>

namespace qt
{
  InfoDialog::InfoDialog(QString const & title, QString const & text, QWidget * parent,
                         QStringList const & buttons)
  : QDialog(parent)
  {
    QIcon icon(":logo.png");
    setWindowIcon(icon);
    setWindowTitle(title);
    setFocusPolicy(Qt::StrongFocus);
    setWindowModality(Qt::WindowModal);

    QVBoxLayout * vBox = new QVBoxLayout();
    QLabel * label = new QLabel(text);
    label->setOpenExternalLinks(true);
    vBox->addWidget(label);

    // this horizontal layout is for buttons
    QHBoxLayout * hBox = new QHBoxLayout();
    hBox->addSpacing(label->width() / 4 * (3.5 - buttons.size()));
    for (int i = 0; i < buttons.size(); ++i)
    {
      QPushButton * button = new QPushButton(buttons[i], this);
      connect(button, SIGNAL(clicked()), this, SLOT(OnButtonClick()));
      hBox->addWidget(button);
    }

    vBox->addLayout(hBox);
    setLayout(vBox);
  }

  void InfoDialog::OnButtonClick()
  {
    // @TODO determine which button is pressed
    done(0);
  }
}
