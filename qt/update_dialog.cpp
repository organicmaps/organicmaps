#include "update_dialog.hpp"

#include <boost/bind.hpp>

#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QTableWidget>
#include <QtGui/QHeaderView>
#include <QtGui/QMessageBox>
#include <QtGui/QProgressBar>

using namespace storage;

enum
{
//  KItemIndexFlag = 0,
  KItemIndexCountry,
  KItemIndexContinent,
  KItemIndexSize,
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
  class QTableWidgetItemWithCustomSorting : public QTableWidgetItem
  {
  public:
    virtual bool operator<(QTableWidgetItem const & other) const
    {
      return data(Qt::UserRole).toULongLong() < other.data(Qt::UserRole).toULongLong();
    }
  };
////////////////////////////////////////////////////////////////////////////////

  UpdateDialog::UpdateDialog(QWidget * parent, Storage & storage)
    : QDialog(parent), m_storage(storage)
  {
    // table with countries list
    m_table = new QTableWidget(0, KNumberOfColumns, this);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    QStringList labels;
    labels << tr("Country") << tr("Continent") << tr("Size");
    m_table->setHorizontalHeaderLabels(labels);
    m_table->horizontalHeader()->setResizeMode(KItemIndexCountry, QHeaderView::Stretch);
    m_table->verticalHeader()->hide();
    m_table->setShowGrid(false);
    connect(m_table, SIGNAL(cellClicked(int, int)), this, SLOT(OnTableClick(int, int)));

    QVBoxLayout * layout = new QVBoxLayout();
    layout->addWidget(m_table);
    setLayout(layout);

    setWindowTitle(tr("Countries"));
    resize(500, 300);

    // we want to receive all download progress and result events
    m_storage.Subscribe(boost::bind(&UpdateDialog::OnCountryChanged, this, _1),
                        boost::bind(&UpdateDialog::OnCountryDownloadProgress, this, _1, _2));
    FillTable();
  }

  UpdateDialog::~UpdateDialog()
  {
    // tell download manager that we're gone...
    m_storage.Unsubscribe();
  }

  /// when user clicks on any map row in the table
  void UpdateDialog::OnTableClick(int row, int /*column*/)
  {
    uint group = m_table->item(row, KItemIndexContinent)->data(Qt::UserRole).toUInt();
    uint country = m_table->item(row, KItemIndexCountry)->data(Qt::UserRole).toUInt();

    switch (m_storage.CountryStatus(TIndex(group, country)))
    {
    case EOnDisk:
      { // aha.. map is already downloaded, so ask user about deleting!
        QMessageBox ask(this);
        ask.setText(tr("Do you want to delete %1?").arg(m_table->item(row, KItemIndexCountry)->text()));
        ask.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        ask.setDefaultButton(QMessageBox::No);
        if (ask.exec() == QMessageBox::Yes)
          m_storage.DeleteCountry(TIndex(group, country));
      }
      break;

    case ENotDownloaded:
    case EDownloadFailed:
      m_storage.DownloadCountry(TIndex(group, country));
      break;

    case EInQueue:
    case EDownloading:
      m_storage.DeleteCountry(TIndex(group, country));
      break;
    }
  }

  int GetRowByGroupAndCountryIndex(QTableWidget & table, TIndex const & index)
  {
    for (int i = 0; i < table.rowCount(); ++i)
    {
      if (table.item(i, KItemIndexContinent)->data(Qt::UserRole).toUInt() == index.first
        && table.item(i, KItemIndexCountry)->data(Qt::UserRole).toUInt() == index.second)
      {
        return i;
      }
    }
    // not found
    return -1;
  }

  /// Changes row's text color
  void SetRowColor(QTableWidget & table, int row, QColor const & color)
  {
    for (int column = 0; column < table.columnCount(); ++column)
    {
      QTableWidgetItem * item = table.item(row, column);
      if (item)
        item->setTextColor(color);
    }
  }

  void UpdateDialog::UpdateRowWithCountryInfo(TIndex const & index)
  {
    int row = GetRowByGroupAndCountryIndex(*m_table, index);
    if (row == -1)
    {
      ASSERT(false, ("Invalid country index"));
      return;
    }

    QColor rowColor;
    TStatus status = m_storage.CountryStatus(index);
    switch (status)
    {
    case ENotDownloaded: rowColor = COLOR_NOTDOWNLOADED; break;
    case EOnDisk: rowColor = COLOR_ONDISK; break;
    case EDownloadFailed: rowColor = COLOR_DOWNLOADFAILED; break;
    case EDownloading: rowColor = COLOR_INPROGRESS; break;
    case EInQueue: rowColor = COLOR_INQUEUE; break;
    }

    // update size column values
    QTableWidgetItem * sizeItem = m_table->item(row, KItemIndexSize);
    if (status == EInQueue)
    {
      sizeItem->setText("In Queue");
    }
    else if (status != EDownloading)
    {
      uint64_t const size = m_storage.CountrySizeInBytes(index);
      if (size > 1000 * 1000)
        sizeItem->setText(QObject::tr("%1 MB").arg(uint(size / (1000 * 1000))));
      else
        sizeItem->setText(QObject::tr("%1 kB").arg(uint((size + 999) / 1000)));
      // needed for column sorting
      sizeItem->setData(Qt::UserRole, QVariant(qint64(size)));
    }
    SetRowColor(*m_table, row, rowColor);
  }

  void UpdateDialog::FillTable()
  {
    m_table->setSortingEnabled(false);
    m_table->clear();

    for (size_t groupIndex = 0; groupIndex < m_storage.GroupsCount(); ++groupIndex)
    {
      for (size_t countryIndex = 0; countryIndex < m_storage.CountriesCountInGroup(groupIndex); ++countryIndex)
      {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        // Country column
        QTableWidgetItem * countryItem = new QTableWidgetItem(m_storage.CountryName(TIndex(groupIndex, countryIndex)).c_str());
        countryItem->setFlags(countryItem->flags() ^ Qt::ItemIsEditable);
        // acts as a key for Storage when user clicks this row
        countryItem->setData(Qt::UserRole, QVariant(uint(countryIndex)));
        m_table->setItem(row, KItemIndexCountry, countryItem);

        // Continent column
        QTableWidgetItem * continentItem = new QTableWidgetItem(m_storage.GroupName(groupIndex).c_str());
        continentItem->setFlags(continentItem->flags() ^ Qt::ItemIsEditable);
        // acts as a key for Storage when user clicks this row
        continentItem->setData(Qt::UserRole, QVariant(uint(groupIndex)));
        m_table->setItem(row, KItemIndexContinent, continentItem);

        // Size column, actual size will be set later
        QTableWidgetItemWithCustomSorting * sizeItem = new QTableWidgetItemWithCustomSorting;
        sizeItem->setFlags(sizeItem->flags() ^ Qt::ItemIsEditable);
        m_table->setItem(row, KItemIndexSize, sizeItem);

        // set color by status and update country size
        UpdateRowWithCountryInfo(TIndex(groupIndex, countryIndex));
      }
    }
    m_table->setSortingEnabled(true);
    m_table->sortByColumn(KItemIndexCountry);
  }

  void UpdateDialog::OnCountryChanged(TIndex const & index)
  {
    UpdateRowWithCountryInfo(index);
  }

  void UpdateDialog::OnCountryDownloadProgress(TIndex const & index, TDownloadProgress const & progress)
  {
    int row = GetRowByGroupAndCountryIndex(*m_table, index);
    if (row != -1)
    {
      m_table->item(row, KItemIndexSize)->setText(
          QString("%1%").arg(progress.first * 100 / progress.second));
    }
    else
    {
      ASSERT(false, ());
    }
  }

}
