#include "qt/editor_dialog.hpp"

#include "qt/qt_common/translations.hpp"

#include "indexer/editable_map_object.hpp"
#include "indexer/feature_utils.hpp"

#include "base/string_utils.hpp"

#include <string>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

namespace
{
constexpr char const * kStreetObjectName = "addr:street";
constexpr char const * kHouseNumberObjectName = "addr:housenumber";
constexpr char const * kPostcodeObjectName = "addr:postcode";
constexpr char const * kInternetObjectName = "internet_access";

QString MetadataLabel(osm::MapObject::MetadataID prop)
{
  using PropID = osm::MapObject::MetadataID;
  switch (prop)
  {
  case PropID::FMD_CUISINE: return qt::Tr("cuisine");
  case PropID::FMD_OPEN_HOURS: return qt::Tr("editor_time_title");
  case PropID::FMD_PHONE_NUMBER: return qt::Tr("phone");
  case PropID::FMD_FAX_NUMBER: return qt::Tr("fax");
  case PropID::FMD_STARS: return qt::Tr("hotel_stars");
  case PropID::FMD_OPERATOR: return qt::Tr("editor_operator");
  case PropID::FMD_WEBSITE: return qt::Tr("website");
  case PropID::FMD_INTERNET: return qt::Tr("internet_access");
  case PropID::FMD_ELE: return qt::Tr("elevation");
  case PropID::FMD_EMAIL: return qt::Tr("email");
  case PropID::FMD_POSTCODE: return qt::Tr("postal_code");
  case PropID::FMD_WIKIPEDIA: return qt::Tr("read_in_wikipedia");
  case PropID::FMD_FLATS: return qt::Tr("apartment_numbers");
  case PropID::FMD_HEIGHT: return qt::Tr("height");
  case PropID::FMD_DENOMINATION: return qt::Tr("religious_denomination");
  case PropID::FMD_BUILDING_LEVELS:
    return qt::Tr("editor_storey_number").arg(osm::EditableMapObject::kMaximumLevelsEditableByUsers);
  case PropID::FMD_LEVEL: return qt::Tr("level");
  case PropID::FMD_CONTACT_FACEBOOK: return "Facebook";
  case PropID::FMD_CONTACT_INSTAGRAM: return "Instagram";
  case PropID::FMD_CONTACT_TWITTER: return "X (Twitter)";
  case PropID::FMD_CONTACT_VK: return "VK";
  case PropID::FMD_CONTACT_LINE: return "LINE";
  case PropID::FMD_WIKIMEDIA_COMMONS: return qt::Tr("wikimedia_commons");
  case PropID::FMD_DRIVE_THROUGH: return qt::Tr("drive_through");
  case PropID::FMD_WEBSITE_MENU: return qt::Tr("website_menu");
  case PropID::FMD_SELF_SERVICE: return qt::Tr("self_service");
  case PropID::FMD_OUTDOOR_SEATING: return qt::Tr("outdoor_seating");
  case PropID::FMD_HERITAGE_WEBSITE: return qt::Tr("heritage_website");
  default: return QString::fromStdString(DebugPrint(prop));
  }
}
}  // namespace

EditorDialog::EditorDialog(QWidget * parent, osm::EditableMapObject & emo) : QDialog(parent), m_feature(emo)
{
  QGridLayout * grid = new QGridLayout();
  int row = 0;

  // Coordinates.
  {
    ms::LatLon const ll = emo.GetLatLon();
    grid->addWidget(new QLabel(qt::Tr("placepage_coordinates")), row, 0);
    QHBoxLayout * coords = new QHBoxLayout();
    coords->addWidget(new QLabel(
        QString::fromStdString(strings::to_string_dac(ll.m_lat, 7) + " " + strings::to_string_dac(ll.m_lon, 7))));
    grid->addLayout(coords, row++, 1);
  }

  // Feature types.
  {
    grid->addWidget(new QLabel(qt::Tr("editor_edit_place_category_title")), row, 0);

    QLabel * label = new QLabel(QString::fromStdString(m_feature.GetLocalizedAllTypes(true)));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    grid->addWidget(label, row++, 1);
  }

  // Names.
  if (emo.IsNameEditable())
  {
    grid->addWidget(new QLabel(qt::Tr("name")), row, 0);

    QGridLayout * namesGrid = new QGridLayout();
    int namesRow = 0;
    for (auto const & ln : emo.GetNamesDataSource().names)
    {
      namesGrid->addWidget(new QLabel(QString::fromUtf8(ln.m_lang.data(), ln.m_lang.size())), namesRow, 0);
      QLineEdit * lineEditName = new QLineEdit(QString::fromStdString(ln.m_name));
      lineEditName->setReadOnly(!emo.IsNameEditable());
      std::string_view const code = StringUtf8Multilang::GetLangByCode(ln.m_code);
      lineEditName->setObjectName(QString::fromUtf8(code.data(), code.size()));
      namesGrid->addWidget(lineEditName, namesRow++, 1);
    }

    grid->addLayout(namesGrid, row++, 1);
  }

  using PropID = osm::MapObject::MetadataID;

  // Address rows.
  if (emo.IsAddressEditable())
  {
    auto nearbyStreets = emo.GetNearbyStreets();
    grid->addWidget(new QLabel(qt::Tr("street")), row, 0);
    QComboBox * cmb = new QComboBox();
    cmb->setEditable(true);

    if (emo.GetStreet().m_defaultName.empty())
      cmb->addItem("");

    for (size_t i = 0; i < nearbyStreets.size(); ++i)
    {
      std::string street = nearbyStreets[i].m_defaultName;
      if (!nearbyStreets[i].m_localizedName.empty())
        street += " / " + nearbyStreets[i].m_localizedName;
      cmb->addItem(street.c_str());
      if (emo.GetStreet() == nearbyStreets[i])
        cmb->setCurrentIndex(static_cast<int>(i));
    }
    cmb->setObjectName(kStreetObjectName);
    grid->addWidget(cmb, row++, 1);

    grid->addWidget(new QLabel(qt::Tr("house_number")), row, 0);
    QLineEdit * houseLineEdit = new QLineEdit(emo.GetHouseNumber().c_str());
    houseLineEdit->setObjectName(kHouseNumberObjectName);
    grid->addWidget(houseLineEdit, row++, 1);

    grid->addWidget(new QLabel(qt::Tr("postal_code")), row, 0);
    QLineEdit * postcodeEdit = new QLineEdit(QString::fromStdString(std::string(emo.GetPostcode())));
    postcodeEdit->setObjectName(kPostcodeObjectName);
    grid->addWidget(postcodeEdit, row++, 1);
  }

  // Editable metadata rows.
  for (auto const prop : emo.GetEditableProperties())
  {
    std::string v;
    switch (prop)
    {
    case PropID::FMD_INTERNET:
    {
      grid->addWidget(new QLabel(MetadataLabel(prop)), row, 0);
      QComboBox * cmb = new QComboBox();
      using feature::Internet;
      cmb->addItem(qt::Tr("not_specified"), static_cast<int>(Internet::Unknown));
      cmb->addItem(qt::Tr("category_wifi"), static_cast<int>(Internet::Wlan));
      cmb->addItem(qt::Tr("wired_internet"), static_cast<int>(Internet::Wired));
      cmb->addItem(qt::Tr("internet_terminal"), static_cast<int>(Internet::Terminal));
      cmb->addItem(qt::Tr("yes"), static_cast<int>(Internet::Yes));
      cmb->addItem(qt::Tr("no"), static_cast<int>(Internet::No));
      cmb->setCurrentIndex(cmb->findData(static_cast<int>(emo.GetInternet())));
      cmb->setObjectName(kInternetObjectName);
      grid->addWidget(cmb, row++, 1);
    }
      continue;
    // Editable values must retain the OSM codes expected by SetCuisines().
    case PropID::FMD_CUISINE: v = strings::JoinStrings(emo.GetCuisines(), ";"); break;
    case PropID::FMD_POSTCODE:  // already set above
      continue;
    default: v = emo.GetMetadata(prop); break;
    }

    QString const fieldName = QString::fromStdString(DebugPrint(prop));
    grid->addWidget(new QLabel(MetadataLabel(prop)), row, 0);
    QLineEdit * lineEdit = new QLineEdit(QString::fromStdString(v));
    lineEdit->setObjectName(fieldName);
    grid->addWidget(lineEdit, row++, 1);
  }

  // Dialog buttons.
  {
    QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EditorDialog::OnSave);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    // Delete button should send custom int return value from dialog.
    QPushButton * deletePOIButton = new QPushButton(qt::Tr("editor_remove_place_button"));
    connect(deletePOIButton, &QPushButton::clicked, this, [this] { done(QDialogButtonBox::DestructiveRole); });
    buttonBox->addButton(deletePOIButton, QDialogButtonBox::DestructiveRole);
    grid->addWidget(buttonBox, row++, 1);
  }

  setLayout(grid);
  setWindowTitle(emo.GetID().IsValid() ? qt::Tr("edit_place") : qt::Tr("contribute_to_osm_add_place"));
}

void EditorDialog::OnSave()
{
  // Store names.
  if (m_feature.IsNameEditable())
  {
    osm::FeatureNames names;
    for (int8_t langCode = StringUtf8Multilang::kDefaultCode; langCode < StringUtf8Multilang::kMaxSupportedLanguages;
         ++langCode)
    {
      std::string_view const lang = StringUtf8Multilang::GetLangByCode(langCode);
      QLineEdit const * le = findChild<QLineEdit const *>(QString::fromUtf8(lang.data(), lang.size()));
      if (le)
        names.Add(langCode, le->text().toStdString());
    }

    m_feature.SetName(std::move(names));
  }

  using PropID = osm::MapObject::MetadataID;

  // Store address.
  if (m_feature.IsAddressEditable())
  {
    m_feature.SetHouseNumber(findChild<QLineEdit *>(kHouseNumberObjectName)->text().toStdString());
    QString const editedStreet = findChild<QComboBox *>(kStreetObjectName)->currentText();
    QStringList const names = editedStreet.split(" / ", Qt::SkipEmptyParts);
    QString const localized = names.size() > 1 ? names[1] : QString();
    if (!names.empty())
      m_feature.SetStreet({names[0].toStdString(), localized.toStdString()});
    else
      m_feature.SetStreet({});

    QLineEdit * editor = findChild<QLineEdit *>(kPostcodeObjectName);
    std::string v = editor->text().toStdString();
    if (osm::EditableMapObject::ValidatePostCode(v))
      m_feature.SetPostcode(v);
  }

  // Store other props.
  for (auto const prop : m_feature.GetEditableProperties())
  {
    if (prop == PropID::FMD_INTERNET)
    {
      QComboBox * cmb = findChild<QComboBox *>(kInternetObjectName);
      m_feature.SetInternet(static_cast<feature::Internet>(cmb->currentData().toInt()));
      continue;
    }
    if (prop == PropID::FMD_POSTCODE)  // already set above
      continue;

    QLineEdit * editor = findChild<QLineEdit *>(QString::fromStdString(DebugPrint(prop)));
    if (!editor)
      continue;

    std::string v = editor->text().toStdString();
    switch (prop)
    {
    case PropID::FMD_CUISINE: m_feature.SetCuisines(strings::Tokenize(v, ";")); break;
    default:
      if (osm::EditableMapObject::IsValidMetadata(prop, v))
        m_feature.SetMetadata(prop, std::move(v));
      else
      {
        /// @todo Show error popup?
        editor->setFocus();
        return;
      }
    }
  }

  accept();
}
