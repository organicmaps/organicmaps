#pragma once
#include "map/framework.hpp"

#include <QtWidgets/QApplication>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QDialog>
#else
  #include <QtWidgets/QDialog>
#endif

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QPushButton;

class Framework;

namespace qt
{
  class UpdateDialog : public QDialog
  {
    Q_OBJECT

  public:
    explicit UpdateDialog(QWidget * parent, Framework & framework);
    virtual ~UpdateDialog();

    /// @name Called from downloader to notify GUI
    //@{
    void OnCountryChanged(storage::TIndex const & index);
    void OnCountryDownloadProgress(storage::TIndex const & index,
                                   pair<int64_t, int64_t> const & progress);
    //@}

    void ShowModal();

  private slots:
    void OnItemClick(QTreeWidgetItem * item, int column);
    void OnCloseClick();

  private:
    void FillTree();
    void UpdateRowWithCountryInfo(storage::TIndex const & index);

    QTreeWidgetItem * CreateTreeItem(storage::TIndex const & index, int value, QTreeWidgetItem * parent);
    int GetChildsCount(storage::TIndex const & index) const;

  private:
    inline storage::Storage & GetStorage() const { return m_framework.Storage(); }

    QTreeWidget * m_tree;
    Framework & m_framework;
    int m_observerSlotId;
  };
} // namespace qt
