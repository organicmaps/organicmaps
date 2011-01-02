#include "update_dialog.hpp"

#include "../base/assert.hpp"

#include <boost/bind.hpp>

#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QTreeWidget>
#include <QtGui/QHeaderView>
#include <QtGui/QMessageBox>
#include <QtGui/QProgressBar>

using namespace storage;

enum
{
//  KItemIndexFlag = 0,
  KColumnIndexCountry,
  KColumnIndexStatus,
  KColumnIndexSize,
  KNumberOfColumns
};

#define COLOR_NOTDOWNLOADED   Qt::black
#define COLOR_ONDISK          Qt::darkGreen
#define COLOR_INPROGRESS      Qt::blue
#define COLOR_DOWNLOADFAILED  Qt::red
#define COLOR_INQUEUE         Qt::gray

namespace qt
{  
///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////
  /// adds custom sorting for "Size" column
  class QTreeWidgetItemWithCustomSorting : public QTreeWidgetItem
  {
  public:
    virtual bool operator<(QTreeWidgetItem const & other) const
    {
      return data(KColumnIndexSize, Qt::UserRole).toULongLong() < other.data(KColumnIndexSize, Qt::UserRole).toULongLong();
    }
  };
////////////////////////////////////////////////////////////////////////////////

  UpdateDialog::UpdateDialog(QWidget * parent, Storage & storage)
    : QDialog(parent), m_storage(storage)
  {
    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(KNumberOfColumns);
    QStringList columnLabels;
    columnLabels << tr("Country") << tr("Status") << tr("Size");
    m_tree->setHeaderLabels(columnLabels);

    connect(m_tree, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(OnItemClick(QTreeWidgetItem *, int)));

    QVBoxLayout * layout = new QVBoxLayout();
    layout->addWidget(m_tree);
    setLayout(layout);

    setWindowTitle(tr("Geographical Regions"));
    resize(600, 500);

    // we want to receive all download progress and result events
    m_storage.Subscribe(boost::bind(&UpdateDialog::OnCountryChanged, this, _1),
                        boost::bind(&UpdateDialog::OnCountryDownloadProgress, this, _1, _2));
    FillTree();
  }

  UpdateDialog::~UpdateDialog()
  {
    // tell download manager that we're gone...
    m_storage.Unsubscribe();
  }

  /// when user clicks on any map row in the table
  void UpdateDialog::OnItemClick(QTreeWidgetItem * item, int /*column*/)
  {
    // calculate index of clicked item
    QList<int> treeIndex;
    {
      QTreeWidgetItem * parent = item;
      while (parent)
      {
        treeIndex.insert(0, parent->data(KColumnIndexCountry, Qt::UserRole).toInt());
        parent = parent->parent();
      }
      while (treeIndex.size() < 3)
        treeIndex.append(-1);
    }

    TIndex const countryIndex(treeIndex[0], treeIndex[1], treeIndex[2]);
    switch (m_storage.CountryStatus(countryIndex))
    {
    case EOnDisk:
      { // aha.. map is already downloaded, so ask user about deleting!
        QMessageBox ask(this);
        ask.setText(tr("Do you want to delete %1?").arg(item->text(KColumnIndexCountry)));
        ask.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        ask.setDefaultButton(QMessageBox::No);
        if (ask.exec() == QMessageBox::Yes)
          m_storage.DeleteCountry(countryIndex);
      }
      break;

    case ENotDownloaded:
    case EDownloadFailed:
      m_storage.DownloadCountry(countryIndex);
      break;

    case EInQueue:
    case EDownloading:
      m_storage.DeleteCountry(countryIndex);
      break;

    default:
      break;
    }
  }

  /// @return can be null if index is invalid
  QTreeWidgetItem * GetTreeItemByIndex(QTreeWidget & tree, TIndex const & index)
  {
    QTreeWidgetItem * item = 0;
    if (index.m_group >= 0 && index.m_group < tree.topLevelItemCount())
    {
      item = tree.topLevelItem(index.m_group);
      ASSERT_EQUAL( item->data(KColumnIndexCountry, Qt::UserRole).toInt(), index.m_group, () );
      if (index.m_country >= 0 && index.m_country < item->childCount())
      {
        item = item->child(index.m_country);
        ASSERT_EQUAL( item->data(KColumnIndexCountry, Qt::UserRole).toInt(), index.m_country, () );
        if (index.m_region >= 0 && index.m_region < item->childCount())
        {
          item = item->child(index.m_region);
          ASSERT_EQUAL( item->data(KColumnIndexCountry, Qt::UserRole).toInt(), index.m_region, () );
        }
      }
    }
    return item;
  }

  /// Changes row's text color
  void SetRowColor(QTreeWidgetItem & item, QColor const & color)
  {
    for (int column = 0; column < item.columnCount(); ++column)
      item.setTextColor(column, color);
  }

  void UpdateDialog::UpdateRowWithCountryInfo(TIndex const & index)
  {
    QTreeWidgetItem * item = GetTreeItemByIndex(*m_tree, index);
    if (item)
    {
      QColor rowColor;
      QString statusString;
      TLocalAndRemoteSize size(0, 0);
      switch (m_storage.CountryStatus(index))
      {
      case ENotDownloaded:
        statusString = tr("Click to download");
        rowColor = COLOR_NOTDOWNLOADED;
        size = m_storage.CountrySizeInBytes(index);
        break;
      case EOnDisk:
        statusString = tr("Installed (click to delete)");
        rowColor = COLOR_ONDISK;
        size = m_storage.CountrySizeInBytes(index);
        break;
      case EDownloadFailed:
        statusString = tr("Download has failed :(");
        rowColor = COLOR_DOWNLOADFAILED;
        size = m_storage.CountrySizeInBytes(index);
        break;
      case EDownloading:
        statusString = tr("Downloading...");
        rowColor = COLOR_INPROGRESS;
        break;
      case EInQueue:
        statusString = tr("Marked for download");
        rowColor = COLOR_INQUEUE;
        size = m_storage.CountrySizeInBytes(index);
        break;
      default:
        break;
      }
      if (statusString.size())
        item->setText(KColumnIndexStatus, statusString);

      if (size.second)
      {
        if (size.second > 1000 * 1000 * 1000)
          item->setText(KColumnIndexSize, QString("%1/%2 GB").arg(
              uint(size.first / (1000 * 1000 * 1000))).arg(uint(size.second / (1000 * 1000 * 1000))));
        else if (size.second > 1000 * 1000)
          item->setText(KColumnIndexSize, QString("%1/%2 MB").arg(
              uint(size.first / (1000 * 1000))).arg(uint(size.second / (1000 * 1000))));
        else
          item->setText(KColumnIndexSize, QString("%1/%2 kB").arg(
              uint((size.first + 999) / 1000)).arg(uint((size.second + 999) / 1000)));
        // needed for column sorting
        item->setData(KColumnIndexSize, Qt::UserRole, QVariant(qint64(size.second)));
      }

      if (statusString.size())
        SetRowColor(*item, rowColor);
    }
  }

  void UpdateDialog::FillTree()
  {
    m_tree->setSortingEnabled(false);
    m_tree->clear();

    for (int group = 0; group < m_storage.CountriesCount(TIndex()); ++group)
    {
      TIndex const grIndex(group);
      QStringList groupText(m_storage.CountryName(grIndex).c_str());
      QTreeWidgetItem * groupItem = new QTreeWidgetItem(groupText);
      groupItem->setData(KColumnIndexCountry, Qt::UserRole, QVariant(group));
      m_tree->addTopLevelItem(groupItem);
      // set color by status and update country size
      UpdateRowWithCountryInfo(grIndex);

      for (int country = 0; country < m_storage.CountriesCount(grIndex); ++country)
      {
        TIndex cIndex(group, country);
        QStringList countryText(m_storage.CountryName(cIndex).c_str());
        QTreeWidgetItem * countryItem = new QTreeWidgetItem(groupItem, countryText);
        countryItem->setData(KColumnIndexCountry, Qt::UserRole, QVariant(country));
        // set color by status and update country size
        UpdateRowWithCountryInfo(cIndex);

        for (int region = 0; region < m_storage.CountriesCount(cIndex); ++region)
        {
          TIndex const rIndex(group, country, region);
          QStringList regionText(m_storage.CountryName(rIndex).c_str());
          QTreeWidgetItem * regionItem = new QTreeWidgetItem(countryItem, regionText);
          regionItem->setData(KColumnIndexCountry, Qt::UserRole, QVariant(region));
          // set color by status and update country size
          UpdateRowWithCountryInfo(rIndex);
        }
      }
    }
//        // Size column, actual size will be set later
//        QTableWidgetItemWithCustomSorting * sizeItem = new QTableWidgetItemWithCustomSorting;
//        sizeItem->setFlags(sizeItem->flags() ^ Qt::ItemIsEditable);
//        m_table->setItem(row, KItemIndexSize, sizeItem);

    m_tree->sortByColumn(KColumnIndexCountry, Qt::AscendingOrder);
    m_tree->setSortingEnabled(true);
    m_tree->header()->setResizeMode(KColumnIndexCountry, QHeaderView::ResizeToContents);
    m_tree->header()->setResizeMode(KColumnIndexStatus, QHeaderView::ResizeToContents);
  }

  void UpdateDialog::OnCountryChanged(TIndex const & index)
  {
    UpdateRowWithCountryInfo(index);
  }

  void UpdateDialog::OnCountryDownloadProgress(TIndex const & index,
                                               TDownloadProgress const & progress)
  {
    QTreeWidgetItem * item = GetTreeItemByIndex(*m_tree, index);
    if (item)
      item->setText(KColumnIndexSize, QString("%1%").arg(progress.first * 100 / progress.second));
  }

}
