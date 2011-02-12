#pragma once

#include <QtGui/QDialog>

namespace qt
{
  /// Simple information dialog with scrollable html content
  class InfoDialog : public QDialog
  {
    Q_OBJECT
  public:
    /// Default constructor creates dialog without any buttons
    explicit InfoDialog(QString const & title, QString const & text, QWidget * parent);
    /// Sets buttons in dialog
    void SetCustomButtons(QStringList const & buttons);

  public slots:
    void OnButtonClick(bool);
  };
}
