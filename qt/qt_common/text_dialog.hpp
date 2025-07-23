#pragma once

#include <QtWidgets/QDialog>

/// A reusable dialog that shows text or HTML.
class TextDialog : public QDialog
{
  Q_OBJECT

public:
  TextDialog(QWidget * parent, QString const & htmlOrText, QString const & title = "");

private slots:
  void OnClose();
};
