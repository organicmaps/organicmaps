#include "testing/testing.hpp"

#include "qt/qt_common/translations.hpp"
#include "qt/update_dialog.hpp"

#include "map/framework.hpp"

#include "storage/queued_country.hpp"

#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"

#include "base/scope_guard.hpp"

#include <QtCore/QMetaObject>
#include <QtWidgets/QAbstractItemDelegate>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleOptionViewItem>
#include <QtWidgets/QTreeWidget>

#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>

namespace
{
class QueuedGuiThread : public base::TaskLoop
{
public:
  PushResult Push(Task && task) override
  {
    std::lock_guard lock(m_mutex);
    m_tasks.push_back(std::move(task));
    m_ready.notify_one();
    return {true, kNoId};
  }

  PushResult Push(Task const & task) override { return Push(Task(task)); }

  Task TakeTask()
  {
    std::unique_lock lock(m_mutex);
    TEST(m_ready.wait_for(lock, std::chrono::seconds(10), [this] { return !m_tasks.empty(); }),
         ("Downloader search did not queue a result"));
    auto task = std::move(m_tasks.front());
    m_tasks.pop_front();
    return task;
  }

private:
  std::mutex m_mutex;
  std::condition_variable m_ready;
  std::deque<Task> m_tasks;
};

// TODO: Would be nice to share this part between similar tests.
void EnsureApp()
{
  if (!QApplication::instance())
  {
    static int argc = 0;
    static QApplication app(argc, nullptr);
  }
}

void CheckChildrenSortedAlphabetically(QTreeWidgetItem const * parent)
{
  for (int i = 1; i < parent->childCount(); ++i)
  {
    auto const * previous = parent->child(i - 1);
    auto const * current = parent->child(i);

    // Compare with Qt's own item comparator (same one QTreeWidget sorts with),
    // valid only because we assert the country column is the sort key below.
    TEST(!(*current < *previous), ("Expected alphabetical order, but got", previous->text(0).toStdString(), "before",
                                   current->text(0).toStdString()));
  }
}

void CheckSubtreeSortedAlphabetically(QTreeWidgetItem const * parent)
{
  CheckChildrenSortedAlphabetically(parent);
  for (int i = 0; i < parent->childCount(); ++i)
    CheckSubtreeSortedAlphabetically(parent->child(i));
}
}  // namespace

UNIT_TEST(UpdateDialog_EntireTreeIsSortedAlphabetically)
{
  EnsureApp();

  Framework framework(FrameworkParams{}, false);
  qt::UpdateDialog dialog(nullptr, framework, QLocale());
  dialog.FillTreeForTesting();

  QTreeWidget const & tree = dialog.GetTreeForTesting();
  TEST_EQUAL(tree.topLevelItemCount(), 1, ());

  QTreeWidgetItem const * root = tree.topLevelItem(0);
  CheckSubtreeSortedAlphabetically(root);

  // #3877: the browse list must sort by the visible country column, not the hidden rank.
  constexpr int kCountryColumn = 0;  // KColumnIndexCountry
  TEST_EQUAL(tree.sortColumn(), kCountryColumn, ());
}

UNIT_TEST(UpdateDialog_RootReflectsDownloadStatus)
{
  EnsureApp();
  Framework framework(FrameworkParams{}, false);
  qt::UpdateDialog dialog(nullptr, framework, QLocale());
  dialog.FillTreeForTesting();
  auto const * root = dialog.GetTreeForTesting().topLevelItem(0);
  int constexpr kStatusColumn = 1;
  auto const initialStatus = root->text(kStatusColumn);

  auto & storage = framework.GetStorage();
  storage::CountryId const id = "Uruguay";
  storage::QueuedCountry country(storage.GetCountryFile(id), id, MapFileType::Map, storage.GetCurrentDataVersion(), {},
                                 std::make_shared<storage::diffs::DiffsDataSource>());
  country.Subscribe(storage);
  country.OnStartDownloading();
  TEST_EQUAL(root->text(kStatusColumn).toStdString(), qt::Tr("downloading").toStdString(), ());
  storage.CancelDownloadNode(id);
  TEST_EQUAL(root->text(kStatusColumn).toStdString(), initialStatus.toStdString(), ());
}

UNIT_TEST(UpdateDialog_QueuedSearchResultAfterDestruction)
{
  EnsureApp();
  auto guiThread = std::make_unique<QueuedGuiThread>();
  auto * queued = guiThread.get();
  GetPlatform().SetGuiThread(std::move(guiThread));
  SCOPE_GUARD(restoreGuiThread, [] { GetPlatform().SetGuiThread(std::make_unique<platform::GuiThread>()); });

  Framework framework(FrameworkParams{}, false);
  framework.GetSearchAPI().OnViewportChanged(m2::RectD(-1, -1, 1, 1));
  auto dialog = std::make_unique<qt::UpdateDialog>(nullptr, framework, QLocale());
  TEST(QMetaObject::invokeMethod(dialog.get(), "OnQueryTextChanged", Qt::DirectConnection,
                                 Q_ARG(QString, QString("France"))),
       ());

  // Hold a real search result until the dialog and its cancellation have finished.
  auto task = queued->TakeTask();
  dialog.reset();
  task();
}

UNIT_TEST(UpdateDialog_LocalizedSizesFitColumn)
{
  EnsureApp();
  auto const direction = QApplication::layoutDirection();
  SCOPE_GUARD(restoreDirection, [direction] { QApplication::setLayoutDirection(direction); });
  QLocale const regionalLocale;
  Framework framework(FrameworkParams{}, false);
  for (std::string const language : {"en", "de", "fr-CA", "ar", "zh-Hans-HK"})
  {
    auto translator = qt::InstallTranslator(*static_cast<QApplication *>(QApplication::instance()), {&language, 1});
    qt::UpdateDialog dialog(nullptr, framework, qt::GetDataSizeLocale(*translator));
    dialog.FillTreeForTesting();
    dialog.show();
    QApplication::processEvents();

    auto const & tree = dialog.GetTreeForTesting();
    QStyleOptionViewItem option;
    option.initFrom(&tree);
    option.widget = &tree;
    int constexpr kSizeColumn = 2;
    auto const * root = tree.topLevelItem(0);
    TEST(root != nullptr && root->childCount() > 0, (language));
    for (int i = 0; i < root->childCount(); ++i)
    {
      auto const * item = root->child(i);
      auto const index = tree.indexFromItem(item, kSizeColumn);
      auto const requiredWidth = tree.itemDelegate()->sizeHint(option, index).width();
      TEST_GREATER_OR_EQUAL(tree.columnWidth(kSizeColumn), requiredWidth,
                            (language, item->text(kSizeColumn).toStdString()));
    }
    TEST(QLocale() == regionalLocale, (language));
  }
}
