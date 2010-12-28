#pragma once

#include "../storage/storage.hpp"

#include <QtGui/QDialog>

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;

namespace qt
{
  class UpdateDialog : public QDialog
  {
    Q_OBJECT

  public:
    UpdateDialog(QWidget * parent, storage::Storage & storage);
    ~UpdateDialog();

    /// @name Called from downloader to notify GUI
    //@{
    void OnCountryChanged(storage::TIndex const & index);
    void OnCountryDownloadProgress(storage::TIndex const & index,
                                   TDownloadProgress const & progress);
    //@}

  private slots:
    void OnItemClick(QTreeWidgetItem * item, int column);

  private:
    void FillTree();
    void UpdateRowWithCountryInfo(storage::TIndex const & index);

  private:
    QTreeWidget * m_tree;
    storage::Storage & m_storage;
   };
} // namespace qt
