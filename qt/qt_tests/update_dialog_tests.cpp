#include "testing/testing.hpp"

#include "qt/qt_common/translations.hpp"
#include "qt/update_dialog.hpp"

#include "map/framework.hpp"

#include "base/scope_guard.hpp"

#include <QtWidgets/QAbstractItemDelegate>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleOptionViewItem>
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
