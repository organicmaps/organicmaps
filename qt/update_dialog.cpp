#include "update_dialog.hpp"
#include "info_dialog.hpp"

#include "../base/assert.hpp"

#include "../map/settings.hpp"

#include <boost/bind.hpp>

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

    string timeString;
    if (!Settings::Get(LAST_CHECK_TIME_KEY, timeString))
      timeString = "Never checked";
    m_label = new QLabel(QString(QObject::tr(LAST_UPDATE_CHECK)) + timeString.c_str(), this);

    m_updateButton = new QPushButton(QObject::tr(CHECK_FOR_UPDATE), this);
    connect(m_updateButton, SIGNAL(clicked()), this, SLOT(OnUpdateClick()));

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(KNumberOfColumns);
    QStringList columnLabels;
    columnLabels << tr("Country") << tr("Status") << tr("Size");
    m_tree->setHeaderLabels(columnLabels);
    connect(m_tree, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(OnItemClick(QTreeWidgetItem *, int)));

    QHBoxLayout * horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(m_label);
    horizontalLayout->addWidget(m_updateButton);
    QVBoxLayout * verticalLayout = new QVBoxLayout();
    verticalLayout->addLayout(horizontalLayout);
    verticalLayout->addWidget(m_tree);
    setLayout(verticalLayout);

    setWindowTitle(tr("Geographical Regions"));
    resize(600, 500);
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

  void UpdateDialog::OnUpdateClick()
  {
    m_updateButton->setText(QObject::tr("Checking for update..."));
    m_updateButton->setDisabled(true);
    m_storage.CheckForUpdate();
  }

  /// Changes row's text color
  void SetRowColor(QTreeWidgetItem & item, QColor const & color)
  {
    for (int column = 0; column < item.columnCount(); ++column)
      item.setTextColor(column, color);
  }

  void UpdateDialog::OnUpdateRequest(storage::TUpdateResult res, string const & description)
  {
    switch (res)
    {
    case ENoAnyUpdateAvailable:
      {
        // @TODO do not show it for automatic update checks
        InfoDialog dlg(tr("No update is available"),
                       tr("At this moment, no new version is available. Please, try again later or "
                          "visit our <a href=\"http://www.mapswithme.com\">site</a> for latest news."),
                       this, QStringList(tr("Ok")));
        dlg.exec();
      }
      break;
    case ENewBinaryAvailable:
      {
        InfoDialog dlg(tr("New version is available!"), description.c_str(), this,
                       QStringList(tr("Postpone update")));
        dlg.exec();
      }
      break;
    case storage::EBinaryCheckFailed:
      {
        InfoDialog dlg(tr("Update check failed"), description.c_str(), this, QStringList(tr("Ok")));
        dlg.exec();
      }
      break;
    }
//    if (updateSize < 0)
//      ;//m_label->setText(QObject::tr("No update is available"));
//    else
//    {
//      QString title(QObject::tr("Update is available"));
//      QString text(readme ? readme : "");
//      if (updateSize / (1000 * 1000 * 1000) > 0)
//        text.append(QObject::tr("\n\nDo you want to perform update and download %1 GB?").arg(
//            uint(updateSize / (1000 * 1000 * 1000))));
//      else if (updateSize / (1000 * 1000) > 0)
//        text.append(QObject::tr("\n\nDo you want to perform update and download %1 MB?").arg(
//            uint(updateSize / (1000 * 1000))));
//      else
//        text.append(QObject::tr("\n\nDo you want to perform update and download %1 kB?").arg(
//            uint((updateSize + 999) / 1000)));
//      if (QMessageBox::Yes == QMessageBox::question(this, title, text, QMessageBox::Yes, QMessageBox::No))
//        m_storage.PerformUpdate();
//    }
    QString labelText(LAST_UPDATE_CHECK);
    QString const textDate = QDateTime::currentDateTime().toString();
    Settings::Set(LAST_CHECK_TIME_KEY, string(textDate.toLocal8Bit().data()));
    m_label->setText(labelText.append(textDate));
    m_updateButton->setText(CHECK_FOR_UPDATE);
    m_updateButton->setDisabled(false);
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

    for (int group = 0; group < static_cast<int>(m_storage.CountriesCount(TIndex())); ++group)
    {
      TIndex const grIndex(group);
      QStringList groupText(m_storage.CountryName(grIndex).c_str());
      QTreeWidgetItem * groupItem = new QTreeWidgetItem(groupText);
      groupItem->setData(KColumnIndexCountry, Qt::UserRole, QVariant(group));
      m_tree->addTopLevelItem(groupItem);
      // set color by status and update country size
      UpdateRowWithCountryInfo(grIndex);

      for (int country = 0; country < static_cast<int>(m_storage.CountriesCount(grIndex)); ++country)
      {
        TIndex cIndex(group, country);
        QStringList countryText(m_storage.CountryName(cIndex).c_str());
        QTreeWidgetItem * countryItem = new QTreeWidgetItem(groupItem, countryText);
        countryItem->setData(KColumnIndexCountry, Qt::UserRole, QVariant(country));
        // set color by status and update country size
        UpdateRowWithCountryInfo(cIndex);

        for (int region = 0; region < static_cast<int>(m_storage.CountriesCount(cIndex)); ++region)
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

  void UpdateDialog::ShowDialog()
  {
    // we want to receive all download progress and result events
    m_storage.Subscribe(bind(&UpdateDialog::OnCountryChanged, this, _1),
                        bind(&UpdateDialog::OnCountryDownloadProgress, this, _1, _2),
                        bind(&UpdateDialog::OnUpdateRequest, this, _1, _2));
    // if called for first time
    if (!m_tree->topLevelItemCount())
      FillTree();

    exec();
  }

}
