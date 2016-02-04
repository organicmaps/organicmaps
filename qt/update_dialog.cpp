#include "qt/update_dialog.hpp"
#include "qt/info_dialog.hpp"

#include "storage/storage_defines.hpp"

#include "platform/settings.hpp"

#include "base/assert.hpp"

#include "std/bind.hpp"

#include <QtCore/QDateTime>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QVBoxLayout>
  #include <QtGui/QHBoxLayout>
  #include <QtGui/QLabel>
  #include <QtGui/QPushButton>
  #include <QtGui/QTreeWidget>
  #include <QtGui/QHeaderView>
  #include <QtGui/QMessageBox>
  #include <QtGui/QProgressBar>
#else
  #include <QtWidgets/QVBoxLayout>
  #include <QtWidgets/QHBoxLayout>
  #include <QtWidgets/QLabel>
  #include <QtWidgets/QPushButton>
  #include <QtWidgets/QTreeWidget>
  #include <QtWidgets/QHeaderView>
  #include <QtWidgets/QMessageBox>
  #include <QtWidgets/QProgressBar>
#endif

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
#define COLOR_OUTOFDATE       Qt::magenta

namespace qt
{
  /// adds custom sorting for "Size" column
  class QTreeWidgetItemWithCustomSorting : public QTreeWidgetItem
  {
  public:
    virtual bool operator<(QTreeWidgetItem const & other) const
    {
      return data(KColumnIndexSize, Qt::UserRole).toULongLong() < other.data(KColumnIndexSize, Qt::UserRole).toULongLong();
    }
  };


  UpdateDialog::UpdateDialog(QWidget * parent, Framework & framework)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      m_framework(framework),
      m_observerSlotId(0)
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
    resize(700, 600);

    // we want to receive all download progress and result events
    m_observerSlotId = GetStorage().Subscribe(bind(&UpdateDialog::OnCountryChanged, this, _1),
                                              bind(&UpdateDialog::OnCountryDownloadProgress, this, _1, _2));
  }

  UpdateDialog::~UpdateDialog()
  {
    // tell download manager that we're gone...
    GetStorage().Unsubscribe(m_observerSlotId);
  }

  /// when user clicks on any map row in the table
  void UpdateDialog::OnItemClick(QTreeWidgetItem * item, int /*column*/)
  {
    // @TODO(bykoianko) Implement when an end user clicks on the map.
    // If the map has been downloaded ask if he wants to delete it.
    // If It hasn't been just download it.
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

  /// @return can be null if countryId is invalid
  QTreeWidgetItem * GetTreeItemByIndex(QTreeWidget & tree, TCountryId const & countryId)
  {
    QTreeWidgetItem * item = 0;
    // @TODO(bykoianko) With the help of MatchedItem find item in tree by countryId.
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

  void UpdateDialog::UpdateRowWithCountryInfo(TCountryId const & countryId)
  {
    QTreeWidgetItem * item = GetTreeItemByIndex(*m_tree, countryId);
    if (item)
    {
      QColor rowColor;
      QString statusString;
      TLocalAndRemoteSize size(0, 0);

      MapOptions const options = MapOptions::MapWithCarRouting;

      Storage const & st = GetStorage();
      switch (m_framework.GetCountryStatus(countryId))
      {
      case TStatus::ENotDownloaded:
        if (st.CountriesCount(countryId) == 0)
        {
          size = st.CountrySizeInBytes(countryId, options);
          ASSERT(size.second > 0, (countryId));
          statusString = tr("Click to download");
        }
        rowColor = COLOR_NOTDOWNLOADED;
        break;

      case TStatus::EOnDisk:
        statusString = tr("Installed (click to delete)");
        rowColor = COLOR_ONDISK;
        size = st.CountrySizeInBytes(countryId, options);
        break;

      case TStatus::EOnDiskOutOfDate:
        statusString = tr("Out of date (click to update or delete)");
        rowColor = COLOR_OUTOFDATE;
        size = st.CountrySizeInBytes(countryId, options);
        break;

      case TStatus::EDownloadFailed:
        statusString = tr("Download has failed");
        rowColor = COLOR_DOWNLOADFAILED;
        size = st.CountrySizeInBytes(countryId, options);
        break;

      case TStatus::EDownloading:
        statusString = tr("Downloading ...");
        rowColor = COLOR_INPROGRESS;
        break;

      case TStatus::EInQueue:
        statusString = tr("Marked for download");
        rowColor = COLOR_INQUEUE;
        size = st.CountrySizeInBytes(countryId, options);
        break;

      default:
        ASSERT(false, ("We shouldn't be here"));
        break;
      }

      if (!statusString.isEmpty())
        item->setText(KColumnIndexStatus, statusString);

      if (size.second > 0)
      {
        int const halfMb = 512 * 1024;
        int const Mb = 1024 * 1024;

        if (size.second > Mb)
        {
          item->setText(KColumnIndexSize, QString("%1/%2 MB").arg(
              uint((size.first + halfMb) / Mb)).arg(uint((size.second + halfMb) / Mb)));
        }
        else
        {
          item->setText(KColumnIndexSize, QString("%1/%2 kB").arg(
              uint((size.first + 1023) / 1024)).arg(uint((size.second + 1023) / 1024)));
        }

        // needed for column sorting
        item->setData(KColumnIndexSize, Qt::UserRole, QVariant(qint64(size.second)));
      }

    // commented out because it looks terrible on black backgrounds
    //  if (!statusString.isEmpty())
    //    SetRowColor(*item, rowColor);
    }
  }

  QTreeWidgetItem * UpdateDialog::CreateTreeItem(TCountryId const & countryId, int value, QTreeWidgetItem * parent)
  {
    QString const text = QString::fromUtf8(GetStorage().CountryName(countryId).c_str());
    QTreeWidgetItem * item = new QTreeWidgetItem(parent, QStringList(text));
    item->setData(KColumnIndexCountry, Qt::UserRole, QVariant(value));

    if (parent == 0)
      m_tree->addTopLevelItem(item);
    return item;
  }

  int UpdateDialog::GetChildsCount(TCountryId const & countryId) const
  {
    return static_cast<int>(GetStorage().CountriesCount(countryId));
  }

  void UpdateDialog::FillTree()
  {
    m_tree->setSortingEnabled(false);
    m_tree->clear();

    // @TODO(bykoianko) Fill m_tree. Use UpdateRowWithCountryInfo for it.

    m_tree->sortByColumn(KColumnIndexCountry, Qt::AscendingOrder);
    m_tree->setSortingEnabled(true);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    m_tree->header()->setResizeMode(KColumnIndexCountry, QHeaderView::ResizeToContents);
    m_tree->header()->setResizeMode(KColumnIndexStatus, QHeaderView::ResizeToContents);
#else
    m_tree->header()->setSectionResizeMode(KColumnIndexCountry, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(KColumnIndexStatus, QHeaderView::ResizeToContents);
#endif
  }

  void UpdateDialog::OnCountryChanged(TCountryId const & countryId)
  {
    UpdateRowWithCountryInfo(countryId);
  }

  void UpdateDialog::OnCountryDownloadProgress(TCountryId const & countryId,
                                               pair<int64_t, int64_t> const & progress)
  {
    QTreeWidgetItem * item = GetTreeItemByIndex(*m_tree, countryId);
    if (item)
      item->setText(KColumnIndexSize, QString("%1%").arg(progress.first * 100 / progress.second));
  }

  void UpdateDialog::ShowModal()
  {
    // if called for first time
    if (!m_tree->topLevelItemCount())
      FillTree();

    exec();
  }
}
