#include "testing/testing.hpp"

#include "qt/editor_dialog.hpp"
#include "qt/qt_common/translations.hpp"

#include "map/framework.hpp"

#include "indexer/classificator.hpp"
#include "indexer/editable_map_object.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPushButton>

#include <array>

namespace editor_dialog_tests
{
QApplication & App()
{
  static int argc = 0;
  static QApplication app(argc, nullptr);
  return app;
}

UNIT_TEST(EditorDialog_InternetValuesSurviveSavingInAnotherLanguage)
{
  std::string const language = "fr";
  auto translator = qt::InstallTranslator(App(), {&language, 1});
  Framework framework(FrameworkParams{}, false);
  using feature::Internet;
  std::array const values = {Internet::Unknown,  Internet::Wlan, Internet::Wired,
                             Internet::Terminal, Internet::Yes,  Internet::No};
  for (size_t i = 0; i < values.size(); ++i)
  {
    osm::EditableMapObject place;
    place.SetType(classif().GetTypeByPath({"amenity", "restaurant"}));
    place.SetInternet(values[i]);
    place.SetEditableProperties({{osm::MapObject::MetadataID::FMD_INTERNET}, false, false, false});

    EditorDialog dialog(nullptr, place);
    auto * field = dialog.findChild<QComboBox *>("internet_access");
    TEST(field != nullptr, ());
    TEST_EQUAL(field->currentData().toInt(), static_cast<int>(values[i]), ());
    TEST_EQUAL(field->itemText(field->findData(static_cast<int>(Internet::Yes))).toStdString(), "Oui", ());

    auto const selected = values[(i + 1) % values.size()];
    field->setCurrentIndex(field->findData(static_cast<int>(selected)));
    dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
    TEST_EQUAL(dialog.result(), QDialog::Accepted, ());
    TEST_EQUAL(place.GetInternet(), selected, ());
  }
}
}  // namespace editor_dialog_tests
