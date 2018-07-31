#include "qt/bookmark_dialog.hpp"

#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"

#include "platform/measurement_utils.hpp"

#include "base/assert.hpp"

#include <QtCore/QFile>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

#include <functional>

using namespace std::placeholders;

namespace qt
{
BookmarkDialog::BookmarkDialog(QWidget * parent, Framework & framework)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
  , m_framework(framework)
{
  setWindowModality(Qt::WindowModal);

  QPushButton * closeButton = new QPushButton(tr("Close"), this);
  closeButton->setDefault(true);
  connect(closeButton, SIGNAL(clicked()), this, SLOT(OnCloseClick()));

  QPushButton * deleteButton = new QPushButton(tr("Delete"), this);
  connect(deleteButton, SIGNAL(clicked()), this, SLOT(OnDeleteClick()));

  QPushButton * importButton = new QPushButton(tr("Import KML/KMZ"), this);
  connect(importButton, SIGNAL(clicked()), this, SLOT(OnImportClick()));

  QPushButton * exportButton = new QPushButton(tr("Export KMZ"), this);
  connect(exportButton, SIGNAL(clicked()), this, SLOT(OnExportClick()));

  m_tree = new QTreeWidget(this);
  m_tree->setColumnCount(2);
  QStringList columnLabels;
  columnLabels << tr("Bookmarks and tracks") << "";
  m_tree->setHeaderLabels(columnLabels);
  m_tree->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  connect(m_tree, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(OnItemClick(QTreeWidgetItem *, int)));

  QHBoxLayout * horizontalLayout = new QHBoxLayout();
  horizontalLayout->addStretch();
  horizontalLayout->addWidget(importButton);
  horizontalLayout->addWidget(exportButton);
  horizontalLayout->addWidget(deleteButton);
  horizontalLayout->addWidget(closeButton);

  QVBoxLayout * verticalLayout = new QVBoxLayout();
  verticalLayout->addWidget(m_tree);
  verticalLayout->addLayout(horizontalLayout);
  setLayout(verticalLayout);

  setWindowTitle(tr("Bookmarks and tracks"));
  resize(700, 600);

  BookmarkManager::AsyncLoadingCallbacks callbacks;
  callbacks.m_onStarted = std::bind(&BookmarkDialog::OnAsyncLoadingStarted, this);
  callbacks.m_onFinished = std::bind(&BookmarkDialog::OnAsyncLoadingFinished, this);
  callbacks.m_onFileSuccess = std::bind(&BookmarkDialog::OnAsyncLoadingFileSuccess, this, _1, _2);
  callbacks.m_onFileError = std::bind(&BookmarkDialog::OnAsyncLoadingFileError, this, _1, _2);
  m_framework.GetBookmarkManager().SetAsyncLoadingCallbacks(std::move(callbacks));
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
}

void BookmarkDialog::OnAsyncLoadingFileError(std::string const & fileName, bool isTemporaryFile)
{
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
  auto const name = QFileDialog::getOpenFileName(this /* parent */, tr("Open KML/KMZ..."),
                                                 QString() /* dir */, "KML/KMZ files (*.kml *.kmz)");
  auto const file = name.toStdString();
  if (file.empty())
    return;

  m_framework.GetBookmarkManager().LoadBookmark(file, false /* isTemporaryFile */);
}

void BookmarkDialog::OnExportClick()
{
  if (m_tree->selectedItems().empty())
  {
    QMessageBox ask(this);
    ask.setIcon(QMessageBox::Information);
    ask.setText(tr("Select one of the bookmark categories to export."));
    ask.addButton(tr("OK"), QMessageBox::NoRole);
    ask.exec();
    return;
  }

  auto const categoryIt = m_categories.find(m_tree->selectedItems().front());
  if (categoryIt == m_categories.cend())
  {
    QMessageBox ask(this);
    ask.setIcon(QMessageBox::Warning);
    ask.setText(tr("Selected item is not a bookmark category."));
    ask.addButton(tr("OK"), QMessageBox::NoRole);
    ask.exec();
    return;
  }

  auto const name = QFileDialog::getSaveFileName(this /* parent */, tr("Export KMZ..."),
                                                 QString() /* dir */, "KMZ files (*.kmz)");
  if (name.isEmpty())
    return;

  m_framework.GetBookmarkManager().PrepareFileForSharing(categoryIt->second,
    [this, name](BookmarkManager::SharingResult const & result)
  {
    if (result.m_code == BookmarkManager::SharingResult::Code::Success)
    {
      QFile::rename(QString(result.m_sharingPath.c_str()), name);

      QMessageBox ask(this);
      ask.setIcon(QMessageBox::Information);
      ask.setText(tr("Bookmarks successfully exported."));
      ask.addButton(tr("OK"), QMessageBox::NoRole);
      ask.exec();
    }
    else
    {
      QMessageBox ask(this);
      ask.setIcon(QMessageBox::Critical);
      ask.setText(tr("Could not export bookmarks: ") + result.m_errorString.c_str());
      ask.addButton(tr("OK"), QMessageBox::NoRole);
      ask.exec();
    }
  });
}

void BookmarkDialog::OnDeleteClick()
{
  auto & bm = m_framework.GetBookmarkManager();
  for (auto item : m_tree->selectedItems())
  {
    auto const categoryIt = m_categories.find(item);
    if (categoryIt != m_categories.cend())
    {
      if (m_categories.size() == 1)
      {
        QMessageBox ask(this);
        ask.setIcon(QMessageBox::Information);
        ask.setText(tr("Could not delete the last category."));
        ask.addButton(tr("OK"), QMessageBox::NoRole);
        ask.exec();
      }
      else
      {
        bm.GetEditSession().DeleteBmCategory(categoryIt->second);
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
  ask.setText(tr("Select category, bookmark or track to delete."));
  ask.addButton(tr("OK"), QMessageBox::NoRole);
  ask.exec();
}

QTreeWidgetItem * BookmarkDialog::CreateTreeItem(std::string const & title, QTreeWidgetItem * parent)
{
  QStringList labels;
  labels << QString(title.c_str()) << tr(parent != nullptr ? "Show on the map" : "");

  QTreeWidgetItem * item = new QTreeWidgetItem(parent, labels);
  item->setData(1, Qt::UserRole, QVariant(title.c_str()));
  item->setData(2, Qt::UserRole, QVariant(tr(parent != nullptr ? "Show on the map" : "")));

  if (parent == nullptr)
    m_tree->addTopLevelItem(item);

  return item;
}

void BookmarkDialog::FillTree()
{
  m_tree->setSortingEnabled(false);
  m_tree->clear();
  m_categories.clear();
  m_bookmarks.clear();
  m_tracks.clear();

  auto categoriesItem = CreateTreeItem("Categories", nullptr);

  auto const & bm = m_framework.GetBookmarkManager();

  if (!bm.IsAsyncLoadingInProgress())
  {
    for (auto catId : bm.GetBmGroupsIdList())
    {
      auto categoryItem = CreateTreeItem(bm.GetCategoryName(catId), categoriesItem);
      m_categories[categoryItem] = catId;

      for (auto bookmarkId : bm.GetUserMarkIds(catId))
      {
        auto const bookmark = bm.GetBookmark(bookmarkId);
        auto name = GetPreferredBookmarkStr(bookmark->GetName());
        if (name.empty())
        {
          name = measurement_utils::FormatLatLon(MercatorBounds::YToLat(bookmark->GetPivot().y),
                                                 MercatorBounds::XToLon(bookmark->GetPivot().x),
                                                 true /* withSemicolon */);
        }
        auto bookmarkItem = CreateTreeItem(name + " (Bookmark)", categoryItem);
        bookmarkItem->setTextColor(0, Qt::blue);
        m_bookmarks[bookmarkItem] = bookmarkId;
      }

      for (auto trackId : bm.GetTrackIds(catId))
      {
        auto const track = bm.GetTrack(trackId);
        auto name = track->GetName();
        if (name.empty())
          name = "No name";
        auto trackItem = CreateTreeItem(name + " (Track)", categoryItem);
        trackItem->setTextColor(0, Qt::darkGreen);
        m_tracks[trackItem] = trackId;
      }
    }
  }
  else
  {
    CreateTreeItem("Loading in progress...", categoriesItem);
  }

  m_tree->expandAll();

  m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
}

void BookmarkDialog::ShowModal()
{
  FillTree();
  exec();
}
}  // namespace qt
