#pragma once

#include "../storage/storage.hpp"

#include <QtGui/QDialog>

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QPushButton;

namespace qt
{
  class UpdateDialog : public QDialog
  {
    Q_OBJECT

  public:
    explicit UpdateDialog(QWidget * parent, storage::Storage & storage);
    ~UpdateDialog();

    /// @name Called from downloader to notify GUI
    //@{
    void OnCountryChanged(storage::TIndex const & index);
    void OnCountryDownloadProgress(storage::TIndex const & index,
                                   pair<int64_t, int64_t> const & progress);
    //@}

    void ShowDialog();

  private slots:
    void OnItemClick(QTreeWidgetItem * item, int column);
    void OnCloseClick();

  private:
    void FillTree();
    void UpdateRowWithCountryInfo(storage::TIndex const & index);

  private:
    QTreeWidget * m_tree;
    storage::Storage & m_storage;
    int m_observerSlotId;
  };
} // namespace qt
