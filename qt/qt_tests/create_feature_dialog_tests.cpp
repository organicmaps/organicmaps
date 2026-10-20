#include "testing/testing.hpp"

#include "qt/create_feature_dialog.hpp"
#include "qt/qt_common/translations.hpp"

#include "map/framework.hpp"

#include "editor/new_feature_categories.hpp"

#include "indexer/classificator.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QListWidget>

namespace create_feature_dialog_tests
{
UNIT_TEST(CreateFeatureDialog_LocalizedCategoryKeepsItsClassifierType)
{
  int argc = 1;
  char appName[] = "create_feature_dialog_tests";
  char * argv[] = {appName, nullptr};
  QApplication app(argc, argv);
  std::string const language = "fr";
  auto translator = qt::InstallTranslator(app, {&language, 1});
  Framework framework(FrameworkParams{}, false);
  auto const categories = framework.GetEditorCategories();
  CreateFeatureDialog dialog(nullptr, categories);
  auto * list = dialog.findChild<QListWidget *>();
  TEST(list != nullptr, ());
  auto const type = classif().GetTypeByPath({"amenity", "restaurant"});
  for (int row = 0; row < list->count(); ++row)
  {
    auto const * item = list->item(row);
    if (item->data(Qt::UserRole).toUInt() != type)
      continue;

    TEST_EQUAL(item->text().toStdString(), "Restaurant", ());
    TEST(QMetaObject::invokeMethod(list, "clicked", Qt::DirectConnection,
                                   Q_ARG(QModelIndex, list->model()->index(row, 0))),
         ());
    TEST_EQUAL(dialog.result(), QDialog::Accepted, ());
    TEST_EQUAL(dialog.GetSelectedType(), type, ());
    return;
  }
  TEST(false, ("Restaurant category was not found"));
}
}  // namespace create_feature_dialog_tests
