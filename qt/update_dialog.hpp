#pragma once

#include "../map/storage.hpp"

#include <QtGui/QDialog>

class QTableWidget;
class QLabel;

namespace qt
{
  class UpdateDialog : public QDialog
  {
    Q_OBJECT

  public:
    UpdateDialog(QWidget * parent, mapinfo::Storage & storage);
    ~UpdateDialog();

    /// @name Called from storage to notify GUI
    //@{
    void OnCountryChanged(mapinfo::TIndex const & index);
    void OnCountryDownloadProgress(mapinfo::TIndex const & index,
                                   TDownloadProgress const & progress);
    //@}

  private slots:
    void OnTableClick(int row, int column);

  private:
    void FillTable();
    void UpdateRowWithCountryInfo(mapinfo::TIndex const & index);

  private:
    QTableWidget * m_table;
    mapinfo::Storage & m_storage;
   };
} // namespace qt
