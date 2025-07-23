#pragma once

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>

class AboutDialog : public QDialog
{
  Q_OBJECT

public:
  explicit AboutDialog(QWidget * parent);
};
