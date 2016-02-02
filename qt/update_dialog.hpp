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
    void OnCountryChanged(storage::TCountryId const & countryId);
    void OnCountryDownloadProgress(storage::TCountryId const & countryId,
                                   pair<int64_t, int64_t> const & progress);
    //@}

    void ShowModal();

  private slots:
    void OnItemClick(QTreeWidgetItem * item, int column);
    void OnCloseClick();

  private:
    void FillTree();
    void UpdateRowWithCountryInfo(storage::TCountryId const & countryId);

    QTreeWidgetItem * CreateTreeItem(storage::TCountryId const & countryId, int value, QTreeWidgetItem * parent);
    int GetChildsCount(storage::TCountryId const & countryId) const;

  private:
    inline storage::Storage & GetStorage() const { return m_framework.Storage(); }

    QTreeWidget * m_tree;
    Framework & m_framework;
    int m_observerSlotId;
  };
} // namespace qt
