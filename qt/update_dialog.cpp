#include "qt/update_dialog.hpp"
#include "qt/qt_common/translations.hpp"

#include "storage/downloader_search_params.hpp"
#include "storage/storage_defines.hpp"

#include "platform/downloader_defines.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <functional>
#include <limits>

#include <QtCore/QDateTime>
#include <QtCore/QLocale>
#include <QtCore/QPointer>
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
  KColumnIndexMatchedBy,
  KColumnIndexPositionInRanking,
  KNumberOfColumns
};

#define COLOR_NOTDOWNLOADED  Qt::black
#define COLOR_ONDISK         Qt::darkGreen
#define COLOR_INPROGRESS     Qt::blue
#define COLOR_DOWNLOADFAILED Qt::red
#define COLOR_INQUEUE        Qt::gray
#define COLOR_OUTOFDATE      Qt::magenta
#define COLOR_MIXED          Qt::yellow

namespace
{
size_t const kInvalidPos = numeric_limits<int32_t>::max();
size_t const kIrrelevantPos = numeric_limits<int32_t>::max() - 1;

bool DeleteNotUploadedEditsConfirmation()
{
  QMessageBox msb;
  msb.setText(qt::Tr("downloader_unuploaded_edits_delete_map"));
  msb.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
  msb.setDefaultButton(QMessageBox::Cancel);
  return QMessageBox::Yes == msb.exec();
}
}  // namespace

namespace qt
{
UpdateDialog::UpdateDialog(QWidget * parent, Framework & framework, QLocale const & dataSizeLocale)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
  , m_framework(framework)
  , m_dataSizeLocale(dataSizeLocale)
  , m_observerSlotId(0)
{
  setWindowModality(Qt::WindowModal);

  QPushButton * closeButton = new QPushButton(Tr("close"), this);
  closeButton->setDefault(true);
  connect(closeButton, &QAbstractButton::clicked, this, &UpdateDialog::OnCloseClick);

  m_tree = new QTreeWidget(this);
  m_tree->setColumnCount(KNumberOfColumns);
  QStringList columnLabels;
  columnLabels << Tr("downloader_country_column") << Tr("downloader_status_column") << Tr("downloader_size_column")
               << Tr("downloader_matched_by_column") << Tr("downloader_rank_column");
  m_tree->setHeaderLabels(columnLabels);

  m_tree->setColumnHidden(KColumnIndexPositionInRanking, true);

  m_tree->header()->setSectionResizeMode(KColumnIndexCountry, QHeaderView::ResizeToContents);
  m_tree->header()->setSectionResizeMode(KColumnIndexStatus, QHeaderView::ResizeToContents);
  m_tree->header()->setSectionResizeMode(KColumnIndexSize, QHeaderView::ResizeToContents);
  m_tree->header()->setSectionResizeMode(KColumnIndexMatchedBy, QHeaderView::ResizeToContents);
  m_tree->header()->setSectionResizeMode(KColumnIndexPositionInRanking, QHeaderView::ResizeToContents);

  connect(m_tree, &QTreeWidget::itemClicked, this, &UpdateDialog::OnItemClick);

  QHBoxLayout * horizontalLayout = new QHBoxLayout();
  horizontalLayout->addStretch();
  horizontalLayout->addWidget(closeButton);

  QLabel * localeLabel = new QLabel(Tr("locale_label"));
  QLineEdit * localeEdit = new QLineEdit(this);
  localeEdit->setText(m_locale.c_str());
  connect(localeEdit, &QLineEdit::textChanged, this, &UpdateDialog::OnLocaleTextChanged);

  QLabel * queryLabel = new QLabel(Tr("search_query_label"));
  QLineEdit * queryEdit = new QLineEdit(this);
  connect(queryEdit, &QLineEdit::textChanged, this, &UpdateDialog::OnQueryTextChanged);

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

  setWindowTitle(Tr("download_maps"));
  resize(900, 600);

  // We want to receive all download progress and result events.
  using namespace std::placeholders;
  m_observerSlotId = GetStorage().Subscribe(bind(&UpdateDialog::OnCountryChanged, this, _1),
                                            bind(&UpdateDialog::OnCountryDownloadProgress, this, _1, _2));
}

UpdateDialog::~UpdateDialog()
{
  m_framework.GetSearchAPI().CancelSearch(search::Mode::Downloader);
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
    ask.setText(Tr("update_or_delete_region_confirmation").arg(countryId.c_str()));
    QPushButton * const btnUpdate = ask.addButton(Tr("refresh"), QMessageBox::ActionRole);
    QPushButton * const btnDelete = ask.addButton(Tr("delete"), QMessageBox::ActionRole);
    QPushButton * const btnCancel = ask.addButton(Tr("cancel"), QMessageBox::NoRole);
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
    ask.setText(Tr("delete_region_confirmation").arg(countryId.c_str()));
    QPushButton * const btnDelete = ask.addButton(Tr("delete"), QMessageBox::ActionRole);
    QPushButton * const btnCancel = ask.addButton(Tr("cancel"), QMessageBox::NoRole);
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
  case NodeStatus::Partly: st.DownloadNode(countryId); break;

  case NodeStatus::InQueue:
  case NodeStatus::Downloading: st.CancelDownloadNode(countryId); break;

  case NodeStatus::Applying:
    // Do nothing.
    break;

  default: ASSERT(false, ("We shouldn't be here")); break;
  }
}

void UpdateDialog::OnCloseClick()
{
  done(0);
}

void UpdateDialog::OnLocaleTextChanged(QString const & text)
{
  m_locale.assign(text.toStdString());
  strings::Trim(m_locale);

  RefillTree();
}

void UpdateDialog::OnQueryTextChanged(QString const & text)
{
  m_query.assign(text.toStdString());

  RefillTree();
}

void UpdateDialog::RefillTree()
{
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

  DownloaderSearchParams params{
      m_query, m_locale,
      // m_onResults
      [self = QPointer<UpdateDialog>(this), timestamp](DownloaderSearchResults const & results)
  {
    // Cancellation does not remove results already queued for the GUI thread.
    if (!self)
      return;

    Filter filter;
    for (size_t i = 0; i < results.m_results.size(); ++i)
    {
      auto const & res = results.m_results[i];
      auto const added = filter.emplace(res.m_countryId, make_pair(i + 1, res.m_matchedName));
      if (!added.second)
        LOG(LWARNING, ("Duplicate CountryId in results for query:", results.m_query));
    }

    self->FillTree(filter, timestamp);
  }};

  m_framework.GetSearchAPI().SearchInDownloader(std::move(params));
}

void UpdateDialog::FillTree(optional<Filter> const & filter, uint64_t timestamp)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (m_fillTreeTimestamp != timestamp)
    return;

  m_tree->setSortingEnabled(false);
  m_tree->clear();
  m_treeItemByCountryId.clear();

  auto const rootId = m_framework.GetStorage().GetRootId();
  FillTreeImpl(nullptr /* parent */, rootId, filter);

  // Expand the root.
  ASSERT_EQUAL(m_tree->topLevelItemCount(), 1, ());
  m_tree->topLevelItem(0)->setExpanded(true);

  // Sort alphabetically when no filter is active; by ranking otherwise.
  m_tree->sortItems(filter ? KColumnIndexPositionInRanking : KColumnIndexCountry, Qt::AscendingOrder);
  m_tree->setSortingEnabled(true);
}

void UpdateDialog::FillTreeImpl(QTreeWidgetItem * parent, CountryId const & countryId, optional<Filter> const & filter)
{
  CountriesVec children;
  GetStorage().GetChildren(countryId, children);

  size_t posInRanking = kInvalidPos;
  string matchedBy;
  if (filter)
  {
    auto const it = filter->find(countryId);
    if (it != filter->end())
    {
      posInRanking = it->second.first;
      matchedBy = it->second.second;
    }
  }
  else
  {
    posInRanking = kIrrelevantPos;
  }

  if (children.empty())
  {
    if (filter && posInRanking == kInvalidPos)
      return;

    QTreeWidgetItem * item = CreateTreeItem(countryId, posInRanking, matchedBy, parent);
    UpdateRowWithCountryInfo(item, countryId);
    return;
  }

  QTreeWidgetItem * item = CreateTreeItem(countryId, posInRanking, matchedBy, parent);
  UpdateRowWithCountryInfo(item, countryId);

  if (filter && posInRanking != kInvalidPos)
  {
    // Filter matches to the group name, do not filter the group.
    for (auto const & child : children)
      FillTreeImpl(item, child, {} /* filter */);

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
    item.setForeground(column, color);
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

  size.first = attrs.m_downloadingProgress.m_bytesDownloaded;
  size.second = attrs.m_mwmSize;

  switch (attrs.m_status)
  {
  case NodeStatus::NotDownloaded:
    ASSERT(size.second > 0, (countryId));
    statusString = Tr("click_to_download");
    rowColor = COLOR_NOTDOWNLOADED;
    break;

  case NodeStatus::OnDisk:
  case NodeStatus::Partly:
    statusString = Tr("installed_click_to_delete");
    rowColor = COLOR_ONDISK;
    break;

  case NodeStatus::OnDiskOutOfDate:
    statusString = Tr("out_of_date_click_to_update_or_delete");
    rowColor = COLOR_OUTOFDATE;
    break;

  case NodeStatus::Error:
    statusString = Tr("country_status_download_failed");
    rowColor = COLOR_DOWNLOADFAILED;
    break;

  case NodeStatus::Downloading:
    statusString = Tr("downloading");
    rowColor = COLOR_INPROGRESS;
    break;

  case NodeStatus::Applying:
    statusString = Tr("downloader_applying").arg(QString::fromStdString(attrs.m_nodeLocalName));
    rowColor = COLOR_INPROGRESS;
    break;

  case NodeStatus::InQueue:
    statusString = Tr("downloader_queued");
    rowColor = COLOR_INQUEUE;
    break;

  default: ASSERT(false, ("We shouldn't be here")); break;
  }

  if (!statusString.isEmpty())
    item->setText(KColumnIndexStatus, statusString);

  if (size.second > 0)
  {
    item->setText(KColumnIndexSize,
                  Tr("downloader_of")
                      .arg(m_dataSizeLocale.formattedDataSize(size.first, 0, QLocale::DataSizeTraditionalFormat),
                           m_dataSizeLocale.formattedDataSize(size.second, 0, QLocale::DataSizeTraditionalFormat)));

    item->setData(KColumnIndexSize, Qt::UserRole, QVariant(static_cast<qint64>(size.second)));
  }

  // Commented out because it looks terrible on black backgrounds.
  //  if (!statusString.isEmpty())
  //    SetRowColor(*item, rowColor);
}

QString UpdateDialog::GetNodeName(CountryId const & countryId)
{
  return QString::fromStdString(GetStorage().GetNodeLocalName(countryId));
}

QTreeWidgetItem * UpdateDialog::CreateTreeItem(CountryId const & countryId, size_t posInRanking, string matchedBy,
                                               QTreeWidgetItem * parent)
{
  QString const text = GetNodeName(countryId);
  QTreeWidgetItem * item = new QTreeWidgetItem(parent, QStringList(text));
  item->setData(KColumnIndexCountry, Qt::UserRole, QVariant(countryId.c_str()));

  auto const pos = QVariant(static_cast<qint64>(posInRanking));
  item->setData(KColumnIndexPositionInRanking, Qt::DisplayRole, pos);

  auto const matched = QVariant(matchedBy.c_str());
  item->setData(KColumnIndexMatchedBy, Qt::DisplayRole, matched);

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
  return item->data(KColumnIndexCountry, Qt::UserRole).toString().toStdString();
}

void UpdateDialog::OnCountryChanged(CountryId const & countryId)
{
  UpdateRowWithCountryInfo(countryId);

  // Storage reports intermediate ancestors but omits the root.
  if (GetStorage().IsLeaf(countryId))
    UpdateRowWithCountryInfo(GetStorage().GetRootId());
}

void UpdateDialog::OnCountryDownloadProgress(CountryId const & countryId, downloader::Progress const & progress)
{
  auto const items = GetTreeItemsByCountryId(countryId);
  QLocale const locale;
  for (auto const item : items)
    item->setText(KColumnIndexSize,
                  locale.toString(static_cast<qulonglong>(progress.m_bytesDownloaded * 100 / progress.m_bytesTotal)) +
                      locale.percent());
}

void UpdateDialog::ShowModal()
{
  // If called for the first time.
  if (!m_tree->topLevelItemCount())
    FillTree({} /* filter */, m_fillTreeTimestamp);

  exec();
}

void UpdateDialog::FillTreeForTesting()
{
  FillTree({} /* filter */, m_fillTreeTimestamp);
}

}  // namespace qt
