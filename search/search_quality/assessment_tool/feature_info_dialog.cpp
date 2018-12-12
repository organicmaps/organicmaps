#include "search/search_quality/assessment_tool/feature_info_dialog.hpp"

#include "search/result.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#include <QtCore/QString>
#include <QtWidgets/QAction>
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

FeatureInfoDialog::FeatureInfoDialog(QWidget * parent, FeatureType & ft,
                                     search::AddressInfo const & address, string const & locale)
  : QDialog(parent)
{
  auto * layout = new QGridLayout();

  {
    auto const & id = ft.GetID();
    CHECK(id.IsValid(), ());

    auto * label = new QLabel("id:");
    auto * content = MakeSelectableLabel(id.GetMwmName() + ", " + strings::to_string(id.m_index));

    AddRow(*layout, label, content);
  }

  {
    auto * label = new QLabel("lat lon:");
    auto const ll = MercatorBounds::ToLatLon(feature::GetCenter(ft));
    auto const ss = strings::to_string_dac(ll.lat, 5) + " " + strings::to_string_dac(ll.lon, 5);
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
      string name;
      if (!ft.GetName(code, name))
        continue;

      auto const * lang = StringUtf8Multilang::GetLangByCode(code);
      CHECK(lang, ("Can't find lang by code:", code));
      auto * label = new QLabel(QString::fromStdString(string(lang) + ":"));
      auto * content = MakeSelectableLabel(name);

      AddRow(*layout, label, content);
    }
  }

  {
    auto const & c = classif();

    vector<string> types;
    ft.ForEachType([&](uint32_t type) { types.push_back(c.GetReadableObjectName(type)); });

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
