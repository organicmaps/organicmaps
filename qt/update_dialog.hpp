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
                                   TDownloadProgress const & progress);
    /// @param updateSize if -1 then no update is available
    /// @param readme optional, can be NULL
    void OnUpdateCheck(int64_t updateSize, char const * readme);
    //@}

  private slots:
    void OnItemClick(QTreeWidgetItem * item, int column);
    void OnButtonClick(bool checked = false);

  private:
    void FillTree();
    void UpdateRowWithCountryInfo(storage::TIndex const & index);

  private:
    QTreeWidget * m_tree;
    //QLabel * m_label;
    QPushButton * m_updateButton;
    storage::Storage & m_storage;
  };
} // namespace qt
