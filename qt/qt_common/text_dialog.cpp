#include "qt/qt_common/text_dialog.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

TextDialog::TextDialog(QWidget * parent, QString const & htmlOrText, QString const & title) : QDialog(parent)
{
  auto * textEdit = new QTextEdit(this);
  textEdit->setReadOnly(true);
  textEdit->setHtml(htmlOrText);

  auto * closeButton = new QPushButton("Close");
  closeButton->setDefault(true);
  connect(closeButton, &QAbstractButton::clicked, this, &TextDialog::OnClose);

  auto * dbb = new QDialogButtonBox();
  dbb->addButton(closeButton, QDialogButtonBox::RejectRole);

  auto * vBoxLayout = new QVBoxLayout(this);
  vBoxLayout->addWidget(textEdit);
  vBoxLayout->addWidget(dbb);
  setLayout(vBoxLayout);

  setWindowTitle(title);

  if (htmlOrText.size() > 10000)
    setWindowState(Qt::WindowMaximized);
  else
    resize(parent->size());
}

void TextDialog::OnClose()
{
  reject();
}
