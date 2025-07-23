#include "qt/editor_dialog.hpp"

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

#include <QtCore/QSignalMapper>

constexpr char const * kStreetObjectName = "addr:street";
constexpr char const * kHouseNumberObjectName = "addr:housenumber";
constexpr char const * kPostcodeObjectName = "addr:postcode";
constexpr char const * kInternetObjectName = "internet_access";

EditorDialog::EditorDialog(QWidget * parent, osm::EditableMapObject & emo) : QDialog(parent), m_feature(emo)
{
  QGridLayout * grid = new QGridLayout();
  int row = 0;

  // Coordinates.
  {
    ms::LatLon const ll = emo.GetLatLon();
    grid->addWidget(new QLabel("Latitude/Longitude:"), row, 0);
    QHBoxLayout * coords = new QHBoxLayout();
    coords->addWidget(new QLabel(
        QString::fromStdString(strings::to_string_dac(ll.m_lat, 7) + " " + strings::to_string_dac(ll.m_lon, 7))));
    grid->addLayout(coords, row++, 1);
  }

  // Feature types.
  {
    grid->addWidget(new QLabel("Type:"), row, 0);

    std::string const raw = DebugPrint(m_feature.GetTypes());
    QLabel * label = new QLabel(QString::fromStdString(raw));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    grid->addWidget(label, row++, 1);
  }

  // Names.
  if (emo.IsNameEditable())
  {
    grid->addWidget(new QLabel(QString("Name:")), row, 0);

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
    grid->addWidget(new QLabel(kStreetObjectName), row, 0);
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

    grid->addWidget(new QLabel(kHouseNumberObjectName), row, 0);
    QLineEdit * houseLineEdit = new QLineEdit(emo.GetHouseNumber().c_str());
    houseLineEdit->setObjectName(kHouseNumberObjectName);
    grid->addWidget(houseLineEdit, row++, 1);

    grid->addWidget(new QLabel(kPostcodeObjectName), row, 0);
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
      grid->addWidget(new QLabel(kInternetObjectName), row, 0);
      QComboBox * cmb = new QComboBox();
      std::string const values[] = {DebugPrint(feature::Internet::Unknown), DebugPrint(feature::Internet::Wlan),
                                    DebugPrint(feature::Internet::Wired),   DebugPrint(feature::Internet::Terminal),
                                    DebugPrint(feature::Internet::Yes),     DebugPrint(feature::Internet::No)};
      for (auto const & v : values)
        cmb->addItem(v.c_str());
      cmb->setCurrentText(DebugPrint(emo.GetInternet()).c_str());
      cmb->setObjectName(kInternetObjectName);
      grid->addWidget(cmb, row++, 1);
    }
      continue;
    case PropID::FMD_CUISINE: v = strings::JoinStrings(emo.GetLocalizedCuisines(), ", "); break;
    case PropID::FMD_POSTCODE:  // already set above
      continue;
    default: v = emo.GetMetadata(prop); break;
    }

    QString const fieldName = QString::fromStdString(DebugPrint(prop));
    grid->addWidget(new QLabel(fieldName), row, 0);
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
    QPushButton * deletePOIButton = new QPushButton("Delete POI");
    QSignalMapper * signalMapper = new QSignalMapper();
    connect(deletePOIButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
    signalMapper->setMapping(deletePOIButton, QDialogButtonBox::DestructiveRole);
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(done(int)));
    buttonBox->addButton(deletePOIButton, QDialogButtonBox::DestructiveRole);
    grid->addWidget(buttonBox, row++, 1);
  }

  setLayout(grid);
  setWindowTitle("OSM Editor");
}

void EditorDialog::OnSave()
{
  // Store names.
  if (m_feature.IsNameEditable())
  {
    StringUtf8Multilang names;
    for (int8_t langCode = StringUtf8Multilang::kDefaultCode; langCode < StringUtf8Multilang::kMaxSupportedLanguages;
         ++langCode)
    {
      std::string_view const lang = StringUtf8Multilang::GetLangByCode(langCode);
      QLineEdit * le = findChild<QLineEdit *>(QString::fromUtf8(lang.data(), lang.size()));
      if (!le)
        continue;

      std::string const name = le->text().toStdString();
      if (!name.empty())
        names.AddString(langCode, name);
    }

    m_feature.SetName(names);
  }

  using PropID = osm::MapObject::MetadataID;

  // Store address.
  if (m_feature.IsAddressEditable())
  {
    m_feature.SetHouseNumber(findChild<QLineEdit *>(kHouseNumberObjectName)->text().toStdString());
    QString const editedStreet = findChild<QComboBox *>(kStreetObjectName)->currentText();
    QStringList const names = editedStreet.split(" / ", Qt::SkipEmptyParts);
    QString const localized = names.size() > 1 ? names.at(1) : QString();
    if (!names.empty())
      m_feature.SetStreet({names.at(0).toStdString(), localized.toStdString()});
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
      m_feature.SetInternet(feature::InternetFromString(cmb->currentText().toStdString()));
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
