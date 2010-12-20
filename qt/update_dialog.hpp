#pragma once

#include "../storage/storage.hpp"

#include <QtGui/QDialog>

class QTableWidget;
class QLabel;

namespace qt
{
  class UpdateDialog : public QDialog
  {
    Q_OBJECT

  public:
    UpdateDialog(QWidget * parent, storage::Storage & storage);
    ~UpdateDialog();

    /// @name Called from storage to notify GUI
    //@{
    void OnCountryChanged(storage::TIndex const & index);
    void OnCountryDownloadProgress(storage::TIndex const & index,
                                   TDownloadProgress const & progress);
    //@}

  private slots:
    void OnTableClick(int row, int column);

  private:
    void FillTable();
    void UpdateRowWithCountryInfo(storage::TIndex const & index);

  private:
    QTableWidget * m_table;
    storage::Storage & m_storage;
   };
} // namespace qt
