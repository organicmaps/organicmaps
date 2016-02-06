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
#define COLOR_MIXED           Qt::yellow

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
  void UpdateDialog::OnItemClick(QTreeWidgetItem * item, int column)
  {
    TCountryId const countryId = GetCountryIdByTreeItem(item);

    Storage & st = GetStorage();

    NodeAttrs attrs;
    st.GetNodeAttrs(countryId, attrs);

    TCountriesVec children;
    st.GetChildren(countryId, children);

    // For group process only click on the column #
    if (!children.empty() && column != 1)
      return;

    switch (attrs.m_status)
    {
    case NodeStatus::OnDiskOutOfDate:
      {
        // map is already downloaded, so ask user about deleting!
        QMessageBox ask(this);
        ask.setIcon(QMessageBox::Question);
        ask.setText(tr("Do you want to update or delete %1?").arg(countryId.c_str()));
        QPushButton * const btnUpdate = ask.addButton(tr("Update"), QMessageBox::ActionRole);
        QPushButton * const btnDelete = ask.addButton(tr("Delete"), QMessageBox::ActionRole);
        QPushButton * const btnCancel = ask.addButton(tr("Cancel"), QMessageBox::NoRole);
        UNUSED_VALUE(btnCancel);

        ask.exec();

        QAbstractButton * const res = ask.clickedButton();

        if (res == btnUpdate)
          st.DownloadNode(countryId);
        else if (res == btnDelete)
          st.DeleteNode(countryId);
      }
      break;

    case NodeStatus::OnDisk:
      {
        // map is already downloaded, so ask user about deleting!
        QMessageBox ask(this);
        ask.setIcon(QMessageBox::Question);
        ask.setText(tr("Do you want to delete %1?").arg(countryId.c_str()));
        QPushButton * const btnDelete = ask.addButton(tr("Delete"), QMessageBox::ActionRole);
        QPushButton * const btnCancel = ask.addButton(tr("Cancel"), QMessageBox::NoRole);
        UNUSED_VALUE(btnCancel);

        ask.exec();

        QAbstractButton * const res = ask.clickedButton();

        if (res == btnDelete)
          st.DeleteNode(countryId);
      }
      break;

    case NodeStatus::NotDownloaded:
    case NodeStatus::Error:
      st.DownloadNode(countryId);
      break;

    case NodeStatus::InQueue:
    case NodeStatus::Downloading:
      st.DeleteNode(countryId);
      break;

    case NodeStatus::Mixed:
      break;

    default:
      ASSERT(false, ("We shouldn't be here"));
      break;
    }
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
    auto const items = GetTreeItemsByCountryId(countryId);
    for (auto const item : items)
      UpdateRowWithCountryInfo(item, countryId);
  }

  void UpdateDialog::UpdateRowWithCountryInfo(QTreeWidgetItem * item, TCountryId const & countryId)
  {
    Storage const & st = GetStorage();

    QColor rowColor;
    QString statusString;
    TLocalAndRemoteSize size(0, 0);

    TCountriesVec children;
    st.GetChildren(countryId, children);

    NodeAttrs attrs;
    st.GetNodeAttrs(countryId, attrs);

    size.first = attrs.m_downloadingMwmSize;
    size.second = attrs.m_mwmSize;

    switch (attrs.m_status)
    {
    case NodeStatus::NotDownloaded:
      ASSERT(size.second > 0, (countryId));
      statusString = tr("Click to download");
      rowColor = COLOR_NOTDOWNLOADED;
      break;

    case NodeStatus::OnDisk:
      statusString = tr("Installed (click to delete)");
      rowColor = COLOR_ONDISK;
      break;

    case NodeStatus::OnDiskOutOfDate:
      statusString = tr("Out of date (click to update or delete)");
      rowColor = COLOR_OUTOFDATE;
      break;

    case NodeStatus::Error:
      statusString = tr("Download has failed");
      rowColor = COLOR_DOWNLOADFAILED;
      break;

    case NodeStatus::Downloading:
      statusString = tr("Downloading ...");
      rowColor = COLOR_INPROGRESS;
      break;

    case NodeStatus::InQueue:
      statusString = tr("Marked for download");
      rowColor = COLOR_INQUEUE;
      break;

    case NodeStatus::Mixed:
      statusString = tr("Mixed status");
      rowColor = COLOR_MIXED;
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

  QTreeWidgetItem * UpdateDialog::CreateTreeItem(TCountryId const & countryId, QTreeWidgetItem * parent)
  {
    // QString const text = QString::fromUtf8(GetStorage().CountryName(countryId).c_str()); // ???
    QString const text = countryId.c_str();
    QTreeWidgetItem * item = new QTreeWidgetItem(parent, QStringList(text));
    item->setData(KColumnIndexCountry, Qt::UserRole, QVariant(countryId.c_str()));

    if (parent == 0)
      m_tree->addTopLevelItem(item);

    m_treeItemByCountryId.insert(make_pair(countryId, item));

    return item;
  }

  vector<QTreeWidgetItem *> UpdateDialog::GetTreeItemsByCountryId(TCountryId const & countryId)
  {
    vector<QTreeWidgetItem *> res;
    auto const p = m_treeItemByCountryId.equal_range(countryId);
    for (auto i = p.first; i != p.second; ++i)
      res.emplace_back(i->second);
    return res;
  }

  storage::TCountryId UpdateDialog::GetCountryIdByTreeItem(QTreeWidgetItem * item)
  {
    return item->data(KColumnIndexCountry, Qt::UserRole).toString().toUtf8().constData();
  }

  void UpdateDialog::FillTreeImpl(QTreeWidgetItem * parent, TCountryId const & countryId)
  {
    QTreeWidgetItem * item = CreateTreeItem(countryId, parent);

    UpdateRowWithCountryInfo(item, countryId);

    TCountriesVec children;
    GetStorage().GetChildren(countryId, children);

    for (auto const & child : children)
      FillTreeImpl(item, child);
  }

  void UpdateDialog::FillTree()
  {
    m_tree->setSortingEnabled(false);
    m_tree->clear();

    TCountryId const rootId = m_framework.Storage().GetRootId();
    FillTreeImpl(nullptr /* parent */, rootId);

    auto const rootItems = GetTreeItemsByCountryId(rootId);
    ASSERT_EQUAL(rootItems.size(), 1, ());
    for (auto const item : rootItems)
      item->setExpanded(true);

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

    // now core does not support callbacks about parent country change, therefore emulate it
    auto const items = GetTreeItemsByCountryId(countryId);
    for (auto const item : items)
    {
      for (auto p = item->parent(); p != nullptr; p = p->parent())
        UpdateRowWithCountryInfo(GetCountryIdByTreeItem(p));
    }
  }

  void UpdateDialog::OnCountryDownloadProgress(TCountryId const & countryId,
                                               pair<int64_t, int64_t> const & progress)
  {
    auto const items = GetTreeItemsByCountryId(countryId);
    for (auto const item : items)
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
