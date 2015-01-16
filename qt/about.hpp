#pragma once

#include <QtWidgets/QApplication>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QDialog>
#else
  #include <QtWidgets/QDialog>
#endif

class AboutDialog : public QDialog
{
  Q_OBJECT
public:
  explicit AboutDialog(QWidget * parent);
};
