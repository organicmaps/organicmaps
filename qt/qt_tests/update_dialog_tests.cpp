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
