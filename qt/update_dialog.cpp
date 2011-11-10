#include "update_dialog.hpp"
#include "info_dialog.hpp"

#include "../platform/settings.hpp"

#include "../base/assert.hpp"

#include "../std/bind.hpp"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QTreeWidget>
#include <QtGui/QHeaderView>
#include <QtGui/QMessageBox>
#include <QtGui/QProgressBar>
#include <QtCore/QDateTime>


#define CHECK_FOR_UPDATE "Check for update"
#define LAST_UPDATE_CHECK "Last update check: "
/// used in settings
#define LAST_CHECK_TIME_KEY "LastUpdateCheckTime"

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
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint), m_storage(storage)
  {
    setWindowModality(Qt::WindowModal);

    QPushButton * closeButton = new QPushButton(QObject::tr("Close"), this);
    closeButton->setDefault(true);
    connect(closeButton, SIGNAL(clicked()), this, SLOT(OnCloseClick()));

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(KNumberOfColumns);
    QStringList columnLabels;
    columnLabels << tr("Country") << tr("Status") << tr("Size");
    m_tree->setHeaderLabels(columnLabels);
    connect(m_tree, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(OnItemClick(QTreeWidgetItem *, int)));

    QHBoxLayout * horizontalLayout = new QHBoxLayout();
    horizontalLayout->addStretch();
    horizontalLayout->addWidget(closeButton);

    QVBoxLayout * verticalLayout = new QVBoxLayout();
    verticalLayout->addWidget(m_tree);
    verticalLayout->addLayout(horizontalLayout);
    setLayout(verticalLayout);

    setWindowTitle(tr("Geographical Regions"));
    resize(600, 500);

    // we want to receive all download progress and result events
    m_storage.Subscribe(bind(&UpdateDialog::OnCountryChanged, this, _1),
                        bind(&UpdateDialog::OnCountryDownloadProgress, this, _1, _2));
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
        treeIndex.append(TIndex::INVALID);
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

  QTreeWidgetItem * MatchedItem(QTreeWidgetItem & parent, int index)
  {
    if (index >= 0)
    {
      for (int i = 0; i < parent.childCount(); ++i)
      {
        QTreeWidgetItem * item = parent.child(i);
        if (index == item->data(KColumnIndexCountry, Qt::UserRole).toInt())
          return item;
      }
    }
    return NULL;
  }

  /// @return can be null if index is invalid
  QTreeWidgetItem * GetTreeItemByIndex(QTreeWidget & tree, TIndex const & index)
  {
    QTreeWidgetItem * item = 0;
    if (index.m_group >= 0)
    {
      for (int i = 0; i < tree.topLevelItemCount(); ++i)
      {
        QTreeWidgetItem * grItem = tree.topLevelItem(i);
        if (index.m_group == grItem->data(KColumnIndexCountry, Qt::UserRole).toInt())
        {
          item = grItem;
          break;
        }
      }
    }
    if (item && index.m_country >= 0)
    {
      item = MatchedItem(*item, index.m_country);
      if (item && index.m_region >= 0)
        item = MatchedItem(*item, index.m_region);
    }
    return item;
  }

  void UpdateDialog::OnCloseClick()
  {
    done(0);
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
      LocalAndRemoteSizeT size(0, 0);
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
        statusString = tr("Download has failed");
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

    for (int group = 0; group < static_cast<int>(m_storage.CountriesCount(TIndex())); ++group)
    {
      TIndex const grIndex(group);
      QStringList groupText(QString::fromUtf8(m_storage.CountryName(grIndex).c_str()));
      QTreeWidgetItem * groupItem = new QTreeWidgetItem(groupText);
      groupItem->setData(KColumnIndexCountry, Qt::UserRole, QVariant(group));
      m_tree->addTopLevelItem(groupItem);
      // set color by status and update country size
      UpdateRowWithCountryInfo(grIndex);

      for (int country = 0; country < static_cast<int>(m_storage.CountriesCount(grIndex)); ++country)
      {
        TIndex cIndex(group, country);
        QStringList countryText(QString::fromUtf8(m_storage.CountryName(cIndex).c_str()));
        QTreeWidgetItem * countryItem = new QTreeWidgetItem(groupItem, countryText);
        countryItem->setData(KColumnIndexCountry, Qt::UserRole, QVariant(country));
        // set color by status and update country size
        UpdateRowWithCountryInfo(cIndex);

        for (int region = 0; region < static_cast<int>(m_storage.CountriesCount(cIndex)); ++region)
        {
          TIndex const rIndex(group, country, region);
          QStringList regionText(QString::fromUtf8(m_storage.CountryName(rIndex).c_str()));
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
                                               pair<int64_t, int64_t> const & progress)
  {
    QTreeWidgetItem * item = GetTreeItemByIndex(*m_tree, index);
    if (item)
    {
//      QString speed;
//      if (progress.m_bytesPerSec > 1000 * 1000)
//        speed = QString(" %1 MB/s").arg(QString::number(static_cast<double>(progress.m_bytesPerSec) / (1000.0 * 1000.0),
//                                                         'f', 1));
//      else if (progress.m_bytesPerSec > 1000)
//        speed = QString(" %1 kB/s").arg(progress.m_bytesPerSec / 1000);
//      else if (progress.m_bytesPerSec >= 0)
//        speed = QString(" %1 B/sec").arg(progress.m_bytesPerSec);

      item->setText(KColumnIndexSize, QString("%1%").arg(progress.first * 100 / progress.second));
//      item->setText(KColumnIndexSize, QString("%1%%2").arg(progress.m_current * 100 / progress.m_total)
//                    .arg(speed));
    }
  }

  void UpdateDialog::ShowDialog()
  {
    // if called for first time
    if (!m_tree->topLevelItemCount())
      FillTree();

    exec();
  }

}
