#pragma once

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>

namespace qt
{
/// Simple information dialog with scrollable html content
/// @note exec() returns 0 if dialog was closed or [1..buttons_count] for any button pressed
class InfoDialog : public QDialog
{
  Q_OBJECT

public:
  explicit InfoDialog(QString const & title, QString const & text, QWidget * parent,
                      QStringList const & buttons = QStringList());
public Q_SLOTS:
  void OnButtonClick1();
  void OnButtonClick2();
  void OnButtonClick3();
};
}  // namespace qt
