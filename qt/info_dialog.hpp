#pragma once

#include <QtGui/QDialog>

namespace qt
{
  /// Simple information dialog with scrollable html content
  class InfoDialog : public QDialog
  {
    Q_OBJECT

  public:
    explicit InfoDialog(QString const & title, QString const & text,
                        QWidget * parent, QStringList const & buttons = QStringList());
  public Q_SLOTS:
    void OnButtonClick();
  };
}
