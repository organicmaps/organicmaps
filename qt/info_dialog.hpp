#pragma once

#include <QtWidgets/QApplication>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QDialog>
#else
  #include <QtWidgets/QDialog>
#endif

namespace qt
{
  /// Simple information dialog with scrollable html content
  /// @note exec() returns 0 if dialog was closed or [1..buttons_count] for any button pressed
  class InfoDialog : public QDialog
  {
    Q_OBJECT

  public:
    explicit InfoDialog(QString const & title, QString const & text,
                        QWidget * parent, QStringList const & buttons = QStringList());
  public Q_SLOTS:
    void OnButtonClick1();
    void OnButtonClick2();
    void OnButtonClick3();
  };
}
