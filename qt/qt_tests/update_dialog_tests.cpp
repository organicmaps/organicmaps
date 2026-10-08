#include "testing/testing.hpp"

#include "qt/update_dialog.hpp"

#include "map/framework.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QTreeWidget>

namespace
{
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
  qt::UpdateDialog dialog(nullptr, framework);
  dialog.FillTreeForTesting();

  QTreeWidget const & tree = dialog.GetTreeForTesting();
  TEST_EQUAL(tree.topLevelItemCount(), 1, ());

  QTreeWidgetItem const * root = tree.topLevelItem(0);
  CheckSubtreeSortedAlphabetically(root);

  // #3877: the browse list must sort by the visible country column, not the hidden rank.
  constexpr int kCountryColumn = 0;  // KColumnIndexCountry
  TEST_EQUAL(tree.sortColumn(), kCountryColumn, ());
}

UNIT_TEST(UpdateDialog_TerrainProgressKeepsIdleMapRow)
{
  EnsureApp();

  Framework framework(FrameworkParams{}, false);
  qt::UpdateDialog dialog(nullptr, framework);
  dialog.FillTreeForTesting();

  auto const & tree = dialog.GetTreeForTesting();
  auto * row = tree.topLevelItem(0);
  while (row->childCount() != 0)
    row = row->child(0);
  auto const id = row->data(0, Qt::UserRole).toString().toStdString();
  auto const status = row->text(1);
  auto const size = row->text(2);
  TEST(!size.isEmpty(), (id));

  // Terrain-only callbacks carry no map transfer bytes and must not replace an idle map size with 0%.
  dialog.OnCountryDownloadProgress(id, {0, 0, 5, 10});
  TEST_EQUAL(row->text(1).toStdString(), status.toStdString(), (id));
  TEST_EQUAL(row->text(2).toStdString(), size.toStdString(), (id));
}
