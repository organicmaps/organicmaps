#include "info_dialog.hpp"

#include <QtGui/QIcon>
#include <QtGui/QTextEdit>
#include <QtGui/QPushButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

#include <QtGui/QLabel>

namespace qt
{
  InfoDialog::InfoDialog(QString const & title, QString const & text, QWidget * parent)
  : QDialog(parent)
  {
    QIcon icon(":logo.png");
    setWindowIcon(icon);
    setWindowTitle(title);

//    QTextEdit * textEdit = new QTextEdit(text, this);
//    textEdit->setReadOnly(true);

    QVBoxLayout * vBox = new QVBoxLayout();
    QLabel * label = new QLabel(text);
    vBox->addWidget(label);
    //vBox->addWidget(textEdit);
    // this horizontal layout is for buttons
    QHBoxLayout * hBox = new QHBoxLayout();
    vBox->addLayout(hBox);

    setLayout(vBox);
  }

  void InfoDialog::OnButtonClick(bool)
  {
    // @TODO determine which button is pressed
    done(0);
  }

  void InfoDialog::SetCustomButtons(QStringList const & buttons)
  {
    QLayout * hBox = layout()->layout();
    // @TODO clear old buttons if any
//    for (int i = 0; i < hBox->count(); ++i)
//    {
//      QLayoutItem * item = hBox->itemAt(i);
//      hBox->removeItem(item);
//      delete item;
//    }

    for (int i = 0; i < buttons.size(); ++i)
    {
      QPushButton * button = new QPushButton(buttons[i]);
      connect(button, SIGNAL(clicked(bool)), this, SLOT(OnButtonClick(bool)));
      hBox->addWidget(button);
    }
  }
}
