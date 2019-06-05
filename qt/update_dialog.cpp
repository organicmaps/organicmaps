#include "qt/update_dialog.hpp"
#include "qt/info_dialog.hpp"

#include "storage/downloader_search_params.hpp"
#include "storage/storage_defines.hpp"

#include "platform/settings.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <functional>
#include <utility>

#include <QtCore/QDateTime>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

using namespace std;
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

namespace
{
bool DeleteNotUploadedEditsConfirmation()
{
  QMessageBox msb;
  msb.setText("Some map edits are not uploaded yet. Are you sure you want to delete map anyway?");
  msb.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
  msb.setDefaultButton(QMessageBox::Cancel);
  return QMessageBox::Yes == msb.exec();
}

bool Matches(CountryId const & countryId, vector<CountryId> const & filter)
{
  return binary_search(filter.begin(), filter.end(), countryId);
}
}  // namespace

namespace qt
{
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

    QLabel * localeLabel = new QLabel(tr("locale:"));
    QLineEdit * localeEdit = new QLineEdit(this);
    localeEdit->setText(m_locale.c_str());
    connect(localeEdit, SIGNAL(textChanged(QString const &)), this,
            SLOT(OnLocaleTextChanged(QString const &)));

    QLabel * queryLabel = new QLabel(tr("search query:"));
    QLineEdit * queryEdit = new QLineEdit(this);
    connect(queryEdit, SIGNAL(textChanged(QString const &)), this,
            SLOT(OnQueryTextChanged(QString const &)));

    QGridLayout * inputLayout = new QGridLayout();
    // widget, row, column
    inputLayout->addWidget(localeLabel, 0, 0);
    inputLayout->addWidget(localeEdit, 0, 1);
    inputLayout->addWidget(queryLabel, 1, 0);
    inputLayout->addWidget(queryEdit, 1, 1);

    QVBoxLayout * verticalLayout = new QVBoxLayout();
    verticalLayout->addLayout(inputLayout);
    verticalLayout->addWidget(m_tree);
    verticalLayout->addLayout(horizontalLayout);
    setLayout(verticalLayout);

    setWindowTitle(tr("Geographical Regions"));
    resize(700, 600);

    // We want to receive all download progress and result events.
    using namespace std::placeholders;
    m_observerSlotId = GetStorage().Subscribe(bind(&UpdateDialog::OnCountryChanged, this, _1),
                                              bind(&UpdateDialog::OnCountryDownloadProgress, this, _1, _2));
  }

  UpdateDialog::~UpdateDialog()
  {
    // Tell download manager that we're gone.
    GetStorage().Unsubscribe(m_observerSlotId);
  }

  /// when user clicks on any map row in the table
  void UpdateDialog::OnItemClick(QTreeWidgetItem * item, int column)
  {
    CountryId const countryId = GetCountryIdByTreeItem(item);

    Storage & st = GetStorage();

    NodeAttrs attrs;
    st.GetNodeAttrs(countryId, attrs);

    CountriesVec children;
    st.GetChildren(countryId, children);

    // For group process only click on the column #.
    if (!children.empty() && column != 1)
      return;

    switch (attrs.m_status)
    {
    case NodeStatus::OnDiskOutOfDate:
      {
        // Map is already downloaded, so ask user about deleting.
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
          st.UpdateNode(countryId);
        else if (res == btnDelete)
        {
          if (!m_framework.HasUnsavedEdits(countryId) || DeleteNotUploadedEditsConfirmation())
            st.DeleteNode(countryId);
        }
      }
      break;

    case NodeStatus::OnDisk:
      {
        // Map is already downloaded, so ask user about deleting.
        QMessageBox ask(this);
        ask.setIcon(QMessageBox::Question);
        ask.setText(tr("Do you want to delete %1?").arg(countryId.c_str()));
        QPushButton * const btnDelete = ask.addButton(tr("Delete"), QMessageBox::ActionRole);
        QPushButton * const btnCancel = ask.addButton(tr("Cancel"), QMessageBox::NoRole);
        UNUSED_VALUE(btnCancel);

        ask.exec();

        QAbstractButton * const res = ask.clickedButton();

        if (res == btnDelete)
        {
          if (!m_framework.HasUnsavedEdits(countryId) || DeleteNotUploadedEditsConfirmation())
            st.DeleteNode(countryId);
        }
      }
      break;

    case NodeStatus::NotDownloaded:
    case NodeStatus::Error:
    case NodeStatus::Partly:
      st.DownloadNode(countryId);
      break;

    case NodeStatus::InQueue:
    case NodeStatus::Downloading:
      st.DeleteNode(countryId);
      break;

    case NodeStatus::Applying:
      // Do nothing.
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

  void UpdateDialog::OnLocaleTextChanged(QString const & text)
  {
    string locale(text.toUtf8().constData());
    strings::Trim(locale);
    m_locale = std::move(locale);

    RefillTree();
  }

  void UpdateDialog::OnQueryTextChanged(QString const & text)
  {
    m_query = string(text.toUtf8().constData());

    RefillTree();
  }

  void UpdateDialog::RefillTree()
  {
    m_tree->clear();
    m_treeItemByCountryId.clear();

    ++m_fillTreeTimestamp;

    string trimmed = m_query;
    strings::Trim(trimmed);

    if (trimmed.empty())
      FillTree({} /* filter */, m_fillTreeTimestamp);
    else
      StartSearchInDownloader();
  }

  void UpdateDialog::StartSearchInDownloader()
  {
    CHECK(!m_query.empty(), ());

    auto const timestamp = m_fillTreeTimestamp;

    DownloaderSearchParams params;
    params.m_query = m_query;
    params.m_inputLocale = m_locale;

    params.m_onResults = [this, timestamp](DownloaderSearchResults const & results) {
      vector<CountryId> filter;
      for (auto const & res : results.m_results)
        filter.push_back(res.m_countryId);

      base::SortUnique(filter);
      FillTree(filter, timestamp);
    };

    m_framework.SearchInDownloader(params);
  }

  void UpdateDialog::FillTree(boost::optional<vector<CountryId>> const & filter, uint64_t timestamp)
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());

    if (m_fillTreeTimestamp != timestamp)
      return;

    m_tree->setSortingEnabled(false);
    m_tree->clear();

    auto const rootId = m_framework.GetStorage().GetRootId();
    FillTreeImpl(nullptr /* parent */, rootId, filter);

    // Expand the root.
    ASSERT_EQUAL(m_tree->topLevelItemCount(), 1, ());
    m_tree->topLevelItem(0)->setExpanded(true);

    // Note. Sorting does not correspond to the ranking of search results and
    // we do not show the matched name in the qt app. So far it's been
    // enough to look at the log output when any of these features were needed.
    m_tree->sortByColumn(KColumnIndexCountry, Qt::AscendingOrder);
    m_tree->setSortingEnabled(true);
    m_tree->header()->setSectionResizeMode(KColumnIndexCountry, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(KColumnIndexStatus, QHeaderView::ResizeToContents);
  }

  void UpdateDialog::FillTreeImpl(QTreeWidgetItem * parent, CountryId const & countryId,
                                  boost::optional<vector<CountryId>> const & filter)
  {
    CountriesVec children;
    GetStorage().GetChildren(countryId, children);

    if (children.empty())
    {
      if (filter && !Matches(countryId, *filter))
        return;

      QTreeWidgetItem * item = CreateTreeItem(countryId, parent);
      UpdateRowWithCountryInfo(item, countryId);
      return;
    }

    QTreeWidgetItem * item = CreateTreeItem(countryId, parent);
    UpdateRowWithCountryInfo(item, countryId);

    if (filter && Matches(countryId, *filter))
    {
      // Filter matches to the group name, do not filter the group.
      for (auto const & child : children)
        FillTreeImpl(item, child, vector<CountryId>());

      return;
    }

    // Filter does not match to the group, but children can.
    for (auto const & child : children)
      FillTreeImpl(item, child, filter);

    // Drop the item if it has no children.
    if (filter && item->childCount() == 0 && parent != nullptr)
    {
      parent->removeChild(item);
      item = nullptr;
    }
  }

  /// Changes row's text color
  void SetRowColor(QTreeWidgetItem & item, QColor const & color)
  {
    for (int column = 0; column < item.columnCount(); ++column)
      item.setTextColor(column, color);
  }

  void UpdateDialog::UpdateRowWithCountryInfo(CountryId const & countryId)
  {
    auto const items = GetTreeItemsByCountryId(countryId);
    for (auto const item : items)
      UpdateRowWithCountryInfo(item, countryId);
  }

  void UpdateDialog::UpdateRowWithCountryInfo(QTreeWidgetItem * item, CountryId const & countryId)
  {
    Storage const & st = GetStorage();

    QColor rowColor;
    QString statusString;
    LocalAndRemoteSize size(0, 0);

    CountriesVec children;
    st.GetChildren(countryId, children);

    NodeAttrs attrs;
    st.GetNodeAttrs(countryId, attrs);

    size.first = attrs.m_downloadingProgress.first;
    size.second = attrs.m_mwmSize;

    switch (attrs.m_status)
    {
    case NodeStatus::NotDownloaded:
      ASSERT(size.second > 0, (countryId));
      statusString = tr("Click to download");
      rowColor = COLOR_NOTDOWNLOADED;
      break;

    case NodeStatus::OnDisk:
    case NodeStatus::Partly:
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

    case NodeStatus::Applying:
      statusString = tr("Applying ...");
      rowColor = COLOR_INPROGRESS;
      break;

    case NodeStatus::InQueue:
      statusString = tr("Marked for download");
      rowColor = COLOR_INQUEUE;
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

      // Needed for column sorting.
      item->setData(KColumnIndexSize, Qt::UserRole, QVariant(qint64(size.second)));
    }

    // Commented out because it looks terrible on black backgrounds.
    //  if (!statusString.isEmpty())
    //    SetRowColor(*item, rowColor);
  }

  QString UpdateDialog::GetNodeName(CountryId const & countryId)
  {
    // QString const text = QString::fromUtf8(GetStorage().CountryName(countryId).c_str()); // ???
    NodeAttrs attrs;
    GetStorage().GetNodeAttrs(countryId, attrs);
    return attrs.m_nodeLocalName.c_str();
  }

  QTreeWidgetItem * UpdateDialog::CreateTreeItem(CountryId const & countryId,
                                                 QTreeWidgetItem * parent)
  {
    QString const text = GetNodeName(countryId);
    QTreeWidgetItem * item = new QTreeWidgetItem(parent, QStringList(text));
    item->setData(KColumnIndexCountry, Qt::UserRole, QVariant(countryId.c_str()));

    if (parent == nullptr)
      m_tree->addTopLevelItem(item);

    m_treeItemByCountryId.insert(make_pair(countryId, item));

    return item;
  }

  vector<QTreeWidgetItem *> UpdateDialog::GetTreeItemsByCountryId(CountryId const & countryId)
  {
    vector<QTreeWidgetItem *> res;
    auto const p = m_treeItemByCountryId.equal_range(countryId);
    for (auto i = p.first; i != p.second; ++i)
      res.emplace_back(i->second);
    return res;
  }

  CountryId UpdateDialog::GetCountryIdByTreeItem(QTreeWidgetItem * item)
  {
    return item->data(KColumnIndexCountry, Qt::UserRole).toString().toUtf8().constData();
  }

  void UpdateDialog::OnCountryChanged(CountryId const & countryId)
  {
    UpdateRowWithCountryInfo(countryId);

    // Now core does not support callbacks about parent country change, therefore emulate it.
    auto const items = GetTreeItemsByCountryId(countryId);
    for (auto const item : items)
    {
      for (auto p = item->parent(); p != nullptr; p = p->parent())
        UpdateRowWithCountryInfo(GetCountryIdByTreeItem(p));
    }
  }

  void UpdateDialog::OnCountryDownloadProgress(CountryId const & countryId,
                                               MapFilesDownloader::Progress const & progress)
  {
    auto const items = GetTreeItemsByCountryId(countryId);
    for (auto const item : items)
      item->setText(KColumnIndexSize, QString("%1%").arg(progress.first * 100 / progress.second));
  }

  void UpdateDialog::ShowModal()
  {
    // If called for the first time.
    if (!m_tree->topLevelItemCount())
      FillTree({} /* filter */, m_fillTreeTimestamp);

    exec();
  }
}  // namespace qt
