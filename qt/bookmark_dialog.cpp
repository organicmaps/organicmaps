#include "qt/bookmark_dialog.hpp"
#include "qt/bookmark_export.hpp"
#include "qt/qt_common/translations.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"

#include "platform/measurement_utils.hpp"

#include <QtCore/QPointer>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

namespace qt
{
using namespace std::placeholders;

BookmarkDialog::BookmarkDialog(QWidget * parent, Framework & framework)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
  , m_framework(framework)
{
  setWindowModality(Qt::WindowModal);

  QPushButton * closeButton = new QPushButton(Tr("close"), this);
  closeButton->setDefault(true);
  connect(closeButton, &QAbstractButton::clicked, this, &BookmarkDialog::OnCloseClick);

  QPushButton * deleteButton = new QPushButton(Tr("delete"), this);
  connect(deleteButton, &QAbstractButton::clicked, this, &BookmarkDialog::OnDeleteClick);

  QPushButton * importButton = new QPushButton(Tr("bookmarks_import"), this);
  connect(importButton, &QAbstractButton::clicked, this, &BookmarkDialog::OnImportClick);

  QPushButton * exportKmzButton = new QPushButton(Tr("export_file"), this);
  connect(exportKmzButton, &QAbstractButton::clicked, this, [this] { OnExportClick(FileType::Kml); });

  QPushButton * exportGpxButton = new QPushButton(Tr("export_file_gpx"), this);
  connect(exportGpxButton, &QAbstractButton::clicked, this, [this] { OnExportClick(FileType::Gpx); });

  QPushButton * exportGeoJsonButton = new QPushButton(Tr("export_file_geojson"), this);
  connect(exportGeoJsonButton, &QAbstractButton::clicked, this, [this] { OnExportClick(FileType::GeoJson); });

  m_tree = new QTreeWidget(this);
  m_tree->setColumnCount(2);
  QStringList columnLabels;
  columnLabels << Tr("bookmarks_and_tracks") << "";
  m_tree->setHeaderLabels(columnLabels);
  m_tree->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  connect(m_tree, &QTreeWidget::itemClicked, this, &BookmarkDialog::OnItemClick);
  QHBoxLayout * horizontalLayout = new QHBoxLayout();
  horizontalLayout->addStretch();
  horizontalLayout->addWidget(importButton);
  horizontalLayout->addWidget(exportKmzButton);
  horizontalLayout->addWidget(exportGpxButton);
  horizontalLayout->addWidget(exportGeoJsonButton);
  horizontalLayout->addWidget(deleteButton);
  horizontalLayout->addWidget(closeButton);

  QVBoxLayout * verticalLayout = new QVBoxLayout();
  verticalLayout->addWidget(m_tree);
  verticalLayout->addLayout(horizontalLayout);
  setLayout(verticalLayout);

  setWindowTitle(Tr("bookmarks_and_tracks"));
  resize(700, 600);

  BookmarkManager::AsyncLoadingCallbacks callbacks;
  callbacks.m_onStarted = std::bind(&BookmarkDialog::OnAsyncLoadingStarted, this);
  callbacks.m_onFinished = std::bind(&BookmarkDialog::OnAsyncLoadingFinished, this);
  callbacks.m_onFileSuccess = std::bind(&BookmarkDialog::OnAsyncLoadingFileSuccess, this, _1, _2);
  callbacks.m_onFileError = std::bind(&BookmarkDialog::OnAsyncLoadingFileError, this, _1, _2);
  m_framework.GetBookmarkManager().SetAsyncLoadingCallbacks(std::move(callbacks));
}

BookmarkDialog::~BookmarkDialog()
{
  // Loading notifications read these callbacks when they reach the GUI thread.
  m_framework.GetBookmarkManager().SetAsyncLoadingCallbacks({});
}

void BookmarkDialog::OnAsyncLoadingStarted()
{
  FillTree();
}

void BookmarkDialog::OnAsyncLoadingFinished()
{
  FillTree();
}

void BookmarkDialog::OnAsyncLoadingFileSuccess(std::string const & fileName, bool isTemporaryFile)
{
  LOG(LINFO, ("OnAsyncLoadingFileSuccess", fileName, isTemporaryFile));
}

void BookmarkDialog::OnAsyncLoadingFileError(std::string const & fileName, bool isTemporaryFile)
{
  LOG(LERROR, ("OnAsyncLoadingFileError", fileName, isTemporaryFile));
}

void BookmarkDialog::OnItemClick(QTreeWidgetItem * item, int column)
{
  if (column != 1)
    return;

  auto const categoryIt = m_categories.find(item);
  if (categoryIt != m_categories.cend())
  {
    done(0);
    m_framework.ShowBookmarkCategory(categoryIt->second);
    return;
  }

  auto const bookmarkIt = m_bookmarks.find(item);
  if (bookmarkIt != m_bookmarks.cend())
  {
    done(0);
    m_framework.ShowBookmark(bookmarkIt->second);
    return;
  }

  auto const trackIt = m_tracks.find(item);
  if (trackIt != m_tracks.cend())
  {
    done(0);
    m_framework.ShowTrack(trackIt->second);
    return;
  }
}

void BookmarkDialog::OnCloseClick()
{
  done(0);
}

void BookmarkDialog::OnImportClick()
{
  auto const files = QFileDialog::getOpenFileNames(this /* parent */, Tr("bookmarks_import"), QString() /* dir */,
                                                   Tr("bookmark_files_filter"));

  for (auto const & name : files)
  {
    auto const file = name.toStdString();
    if (file.empty())
      continue;

    m_framework.GetBookmarkManager().LoadBookmark(file, false /* isTemporaryFile */);
  }
}

void BookmarkDialog::OnExportClick(FileType exportedFileType)
{
  auto const selected = m_tree->selectedItems();
  if (selected.empty())
  {
    QMessageBox ask(this);
    ask.setIcon(QMessageBox::Information);
    ask.setText(Tr("select_bookmark_category_to_export"));
    ask.addButton(Tr("ok"), QMessageBox::NoRole);
    ask.exec();
    return;
  }

  auto const categoryIt = m_categories.find(selected.front());
  if (categoryIt == m_categories.cend())
  {
    QMessageBox ask(this);
    ask.setIcon(QMessageBox::Warning);
    ask.setText(Tr("selected_item_not_bookmark_category"));
    ask.addButton(Tr("ok"), QMessageBox::NoRole);
    ask.exec();
    return;
  }

  // The save dialog runs an event loop that can rebuild the category tree.
  auto const categoryId = categoryIt->second;
  QString caption, filter;
  switch (exportedFileType)
  {
  case FileType::Gpx:
    caption = Tr("export_file_gpx");
    filter = Tr("gpx_filter");
    break;
  case FileType::GeoJson:
    caption = Tr("export_file_geojson");
    filter = Tr("geojson_filter");
    break;
  default: caption = Tr("export_file"); filter = Tr("kmz_filter");
  }

  auto const name = QFileDialog::getSaveFileName(this /* parent */, caption, QString() /* dir */, filter);
  if (name.isEmpty())
    return;

  m_framework.GetBookmarkManager().PrepareFileForSharing(
      {categoryId}, [self = QPointer<BookmarkDialog>(this), name](BookmarkManager::SharingResult const & result)
  {
    auto const success = result.m_code == BookmarkManager::SharingResult::Code::Success &&
                         SaveExportedFile(QString::fromStdString(result.m_sharingPath), name);
    if (!self)
      return;

    QMessageBox ask(self.data());
    ask.setIcon(success ? QMessageBox::Information : QMessageBox::Critical);
    ask.setText(success ? Tr("bookmarks_exported")
                        : Tr("bookmarks_export_error")
                              .arg(result.m_errorString.empty() ? Tr("cannot_save_selected_file")
                                                                : QString::fromStdString(result.m_errorString)));
    ask.addButton(Tr("ok"), QMessageBox::NoRole);
    ask.exec();
  }, exportedFileType);
}

void BookmarkDialog::OnDeleteClick()
{
  auto & bm = m_framework.GetBookmarkManager();
  for (auto const item : m_tree->selectedItems())
  {
    auto const categoryIt = m_categories.find(item);
    if (categoryIt != m_categories.cend())
    {
      if (m_categories.size() == 1)
      {
        QMessageBox ask(this);
        ask.setIcon(QMessageBox::Information);
        ask.setText(Tr("cannot_delete_last_bookmark_category"));
        ask.addButton(Tr("ok"), QMessageBox::NoRole);
        ask.exec();
      }
      else
      {
        bm.GetEditSession().DeleteBmCategory(categoryIt->second, true);
        FillTree();
      }
      return;
    }

    auto const bookmarkIt = m_bookmarks.find(item);
    if (bookmarkIt != m_bookmarks.cend())
    {
      bm.GetEditSession().DeleteBookmark(bookmarkIt->second);
      FillTree();
      return;
    }

    auto const trackIt = m_tracks.find(item);
    if (trackIt != m_tracks.cend())
    {
      bm.GetEditSession().DeleteTrack(trackIt->second);
      FillTree();
      return;
    }
  }

  QMessageBox ask(this);
  ask.setIcon(QMessageBox::Warning);
  ask.setText(Tr("select_bookmark_item_to_delete"));
  ask.addButton(Tr("ok"), QMessageBox::NoRole);
  ask.exec();
}

QTreeWidgetItem * BookmarkDialog::CreateTreeItem(QString const & title, QTreeWidgetItem * parent)
{
  QStringList labels;
  labels << title << (parent != nullptr ? Tr("zoom_to_country") : QString());

  QTreeWidgetItem * item = new QTreeWidgetItem(labels);
  if (parent)
    parent->addChild(item);

  return item;
}

void BookmarkDialog::FillTree()
{
  m_tree->setSortingEnabled(false);
  m_tree->clear();
  m_categories.clear();
  m_bookmarks.clear();
  m_tracks.clear();

  auto categoriesItem = CreateTreeItem(Tr("categories"), nullptr);

  auto const & bm = m_framework.GetBookmarkManager();

  if (!bm.IsAsyncLoadingInProgress())
  {
    for (auto catId : bm.GetUnsortedBmGroupsIdList())
    {
      auto categoryItem = CreateTreeItem(QString::fromStdString(bm.GetCategoryName(catId)), categoriesItem);
      m_categories[categoryItem] = catId;

      for (auto bookmarkId : bm.GetUserMarkIds(catId))
      {
        auto const bookmark = bm.GetBookmark(bookmarkId);
        auto name = GetPreferredBookmarkStr(bookmark->GetName());
        if (name.empty())
        {
          name = measurement_utils::FormatLatLon(mercator::YToLat(bookmark->GetPivot().y),
                                                 mercator::XToLon(bookmark->GetPivot().x), true /* withComma */);
        }
        auto bookmarkItem =
            CreateTreeItem(Tr("bookmark_title_with_type").arg(QString::fromStdString(name)), categoryItem);
        m_bookmarks[bookmarkItem] = bookmarkId;
      }

      for (auto trackId : bm.GetTrackIds(catId))
      {
        auto const track = bm.GetTrack(trackId);
        auto const name = track->GetName().empty() ? Tr("unnamed_track") : QString::fromStdString(track->GetName());
        auto trackItem = CreateTreeItem(Tr("track_title_with_type").arg(name), categoryItem);
        trackItem->setForeground(0, Qt::darkGreen);
        m_tracks[trackItem] = trackId;
      }
      for (auto areaId : bm.GetAreaIds(catId))
      {
        auto const & area = *bm.GetArea(areaId);
        auto name = QString::fromStdString(GetPreferredBookmarkStr(area.m_name));
        if (name.isEmpty())
          name = Tr("unnamed_area");
        auto * areaItem = CreateTreeItem(Tr("area_title_with_type").arg(name), categoryItem);
        // Areas have category controls but no individual show/delete actions.
        areaItem->setText(1, {});
        areaItem->setFlags(areaItem->flags() & ~Qt::ItemIsSelectable);
      }
    }
  }
  else
  {
    CreateTreeItem(Tr("please_wait"), categoriesItem);
  }

  m_tree->addTopLevelItem(categoriesItem);
  m_tree->expandAll();
  m_tree->setCurrentItem(categoriesItem);

  m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
}

void BookmarkDialog::ShowModal()
{
  FillTree();
  exec();
}
}  // namespace qt
