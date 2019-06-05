#pragma once

#include "map/framework.hpp"

#include "base/thread_checker.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>

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
    void OnCountryChanged(storage::CountryId const & countryId);
    void OnCountryDownloadProgress(storage::CountryId const & countryId,
                                   storage::MapFilesDownloader::Progress const & progress);
    //@}

    void ShowModal();

  private slots:
    void OnItemClick(QTreeWidgetItem * item, int column);
    void OnCloseClick();
    void OnLocaleTextChanged(QString const & text);
    void OnQueryTextChanged(QString const & text);

  private:
    void RefillTree();
    void StartSearchInDownloader();

    // Adds only those countries present in |filter|.
    // Calls whose timestamp is not the latest are discarded.
    void FillTree(boost::optional<std::vector<storage::CountryId>> const & filter,
                  uint64_t timestamp);
    void FillTreeImpl(QTreeWidgetItem * parent, storage::CountryId const & countryId,
                      boost::optional<std::vector<storage::CountryId>> const & filter);

    void UpdateRowWithCountryInfo(storage::CountryId const & countryId);
    void UpdateRowWithCountryInfo(QTreeWidgetItem * item, storage::CountryId const & countryId);
    QString GetNodeName(storage::CountryId const & countryId);

    QTreeWidgetItem * CreateTreeItem(storage::CountryId const & countryId,
                                     QTreeWidgetItem * parent);
    std::vector<QTreeWidgetItem *> GetTreeItemsByCountryId(storage::CountryId const & countryId);
    storage::CountryId GetCountryIdByTreeItem(QTreeWidgetItem *);

    inline storage::Storage & GetStorage() const { return m_framework.GetStorage(); }

    QTreeWidget * m_tree;
    Framework & m_framework;
    int m_observerSlotId;

    // Params of the queries to the search engine.
    std::string m_query;
    std::string m_locale = "en";
    uint64_t m_fillTreeTimestamp = 0;

    std::unordered_multimap<storage::CountryId, QTreeWidgetItem *> m_treeItemByCountryId;

    DECLARE_THREAD_CHECKER(m_threadChecker);
  };
  }  // namespace qt
