#include "search/search_quality/assessment_tool/feature_info_dialog.hpp"

#include "indexer/classificator.hpp"
#include "indexer/map_object.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#include <QtCore/QString>
#include <QtGui/QAction>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

using namespace std;

namespace
{
QLabel * MakeSelectableLabel(string const & s)
{
  auto * result = new QLabel(QString::fromStdString(s));
  result->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return result;
}
}  // namespace

FeatureInfoDialog::FeatureInfoDialog(QWidget * parent, osm::MapObject const & mapObject,
                                     search::ReverseGeocoder::Address const & address, string const & locale)
  : QDialog(parent)
{
  auto * layout = new QGridLayout();

  {
    auto const & id = mapObject.GetID();
    CHECK(id.IsValid(), ());

    auto * label = new QLabel("id:");
    auto * content = MakeSelectableLabel(id.GetMwmName() + ", " + strings::to_string(id.m_index));

    AddRow(*layout, label, content);
  }

  {
    auto * label = new QLabel("lat lon:");
    auto const ll = mapObject.GetLatLon();
    auto const ss = strings::to_string_dac(ll.m_lat, 5) + " " + strings::to_string_dac(ll.m_lon, 5);
    auto * content = MakeSelectableLabel(ss);

    AddRow(*layout, label, content);
  }

  {
    int8_t const localeCode = StringUtf8Multilang::GetLangIndex(locale);
    vector<int8_t> codes = {{StringUtf8Multilang::kDefaultCode, StringUtf8Multilang::kEnglishCode}};
    if (localeCode != StringUtf8Multilang::kUnsupportedLanguageCode &&
        ::find(codes.begin(), codes.end(), localeCode) == codes.end())
    {
      codes.push_back(localeCode);
    }

    for (auto const & code : codes)
    {
      string_view name;
      if (!mapObject.GetNameMultilang().GetString(code, name))
        continue;

      auto const lang = StringUtf8Multilang::GetLangByCode(code);
      CHECK(!lang.empty(), ("Can't find lang by code:", code));
      auto * label = new QLabel(QString::fromStdString(std::string{lang} + ":"));
      auto * content = MakeSelectableLabel(std::string{name});

      AddRow(*layout, label, content);
    }
  }

  {
    auto const & c = classif();

    vector<string> types;
    for (auto type : mapObject.GetTypes())
      types.push_back(c.GetReadableObjectName(type));

    if (!types.empty())
    {
      auto * label = new QLabel("types:");
      auto * content = MakeSelectableLabel(strings::JoinStrings(types, " " /* delimiter */));

      AddRow(*layout, label, content);
    }
  }

  {
    auto * label = new QLabel("address:");
    auto * content = MakeSelectableLabel(address.FormatAddress());

    AddRow(*layout, label, content);
  }

  auto * ok = new QPushButton("OK");
  ok->setDefault(true);
  connect(ok, &QPushButton::clicked, [&]() { accept(); });
  AddRow(*layout, ok);

  setLayout(layout);
}
