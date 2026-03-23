#include "qt/update_dialog.hpp"

#include <gtest/gtest.h>

#include <QApplication>
#include <QTreeWidget>

#include <string>

namespace
{
QApplication * GetOrCreateApp()
{
  if (QApplication::instance() != nullptr) {
    return static_cast<QApplication *>(QApplication::instance());
  }

  int argc = 0;
  char ** argv = nullptr;
  return new QApplication(argc, argv);
}

void ExpectChildrenSortedAlphabetically(QTreeWidgetItem const * parent)
{
  ASSERT_NE(parent, nullptr);

  for (int i = 1; i < parent->childCount(); ++i)
  {
    auto const * previous = parent->child(i - 1);
    auto const * current = parent->child(i);

    ASSERT_NE(previous, nullptr);
    ASSERT_NE(current, nullptr);

    EXPECT_FALSE(*current < *previous)
        << "Expected sorted order, but got \""
        << previous->text(0).toStdString() << "\" before \""
        << current->text(0).toStdString() << "\"";
  }
}

void ExpectSubtreeSortedAlphabetically(QTreeWidgetItem const * parent)
{
  ASSERT_NE(parent, nullptr);

  ExpectChildrenSortedAlphabetically(parent);

  for (int i = 0; i < parent->childCount(); ++i) {
    ExpectSubtreeSortedAlphabetically(parent->child(i));
  }
}
}  // namespace

// Extends the regression coverage to nested groups to ensure sorting is
// preserved throughout the full update tree, not only at the root level.
TEST(UpdateDialogRegression, EntireTreeIsSortedAlphabetically)
{
  GetOrCreateApp();

  Framework framework(FrameworkParams{}, false);
  qt::UpdateDialog dialog(nullptr, framework);
  dialog.FillTreeForTesting();

  QTreeWidget const & tree = dialog.GetTreeForTesting();

  ASSERT_EQ(tree.topLevelItemCount(), 1);

  QTreeWidgetItem const * root = tree.topLevelItem(0);
  ASSERT_NE(root, nullptr);

  ExpectSubtreeSortedAlphabetically(root);
}
