#include "qt/editor_dialog.hpp"

#include "map/framework.hpp"

#include "search/result.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/osm_editor.hpp"

#include "base/collection_cast.hpp"

#include "std/set.hpp"
#include "std/vector.hpp"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <QtCore/QSignalMapper>

using feature::Metadata;

constexpr char const * kStreetObjectName = "addr:street";
constexpr char const * kHouseNumberObjectName = "addr:housenumber";

EditorDialog::EditorDialog(QWidget * parent, FeatureType & feature, Framework & frm) : QDialog(parent)
{
  osm::Editor & editor = osm::Editor::Instance();

  QVBoxLayout * vLayout = new QVBoxLayout();

  // Zero uneditable row: coordinates.
  ms::LatLon const ll = MercatorBounds::ToLatLon(feature::GetCenter(feature));
  QHBoxLayout * coordinatesRow = new QHBoxLayout();
  coordinatesRow->addWidget(new QLabel("Latitude, Longitude:"));
  QLabel * coords = new QLabel(QString::fromStdString(strings::to_string_dac(ll.lat, 6) +
                                                      "," + strings::to_string_dac(ll.lon, 6)));
  coords->setTextInteractionFlags(Qt::TextSelectableByMouse);
  coordinatesRow->addWidget(coords);
  vLayout->addLayout(coordinatesRow);

  // First uneditable row: feature types.
  string strTypes;
  feature.ForEachType([&strTypes](uint32_t type)
  {
    strTypes += classif().GetReadableObjectName(type) + " ";
  });
  QHBoxLayout * typesRow = new QHBoxLayout();
  typesRow->addWidget(new QLabel("Types:"));
  typesRow->addWidget(new QLabel(QString::fromStdString(strTypes)));
  vLayout->addLayout(typesRow);

  osm::EditableProperties const editable = editor.GetEditableProperties(feature);

  // Rows block: Name(s) label(s) and text input.
  char const * defaultLangStr = StringUtf8Multilang::GetLangByCode(StringUtf8Multilang::DEFAULT_CODE);
  // Default name editor is always displayed, even if feature name is empty.
  QHBoxLayout * defaultNameRow = new QHBoxLayout();
  defaultNameRow->addWidget(new QLabel(QString("name")));
  QLineEdit * defaultNamelineEdit = new QLineEdit();
  defaultNamelineEdit->setReadOnly(!editable.m_name);
  defaultNamelineEdit->setObjectName(defaultLangStr);
  defaultNameRow->addWidget(defaultNamelineEdit);
  vLayout->addLayout(defaultNameRow);

  feature.ForEachName([&](int8_t langCode, string const & name) -> bool
  {
    if (langCode == StringUtf8Multilang::DEFAULT_CODE)
      defaultNamelineEdit->setText(QString::fromStdString(name));
    else
    {
      QHBoxLayout * nameRow = new QHBoxLayout();
      char const * langStr = StringUtf8Multilang::GetLangByCode(langCode);
      nameRow->addWidget(new QLabel(QString("name:") + langStr));
      QLineEdit * lineEditName = new QLineEdit(QString::fromStdString(name));
      lineEditName->setReadOnly(!editable.m_name);
      lineEditName->setObjectName(langStr);
      nameRow->addWidget(lineEditName);
      vLayout->addLayout(nameRow);
    }
    return true; // true is needed to enumerate all languages.
  });

  // Address rows.
  vector<string> nearbyStreets = frm.GetNearbyFeatureStreets(feature);
  // If feature does not have a specified street, display empty combo box.
  search::AddressInfo const info = frm.GetFeatureAddressInfo(feature);
  if (info.m_street.empty())
    nearbyStreets.insert(nearbyStreets.begin(), "");
  QHBoxLayout * streetRow = new QHBoxLayout();
  streetRow->addWidget(new QLabel(QString(kStreetObjectName)));
  QComboBox * cmb = new QComboBox();
  for (auto const & street : nearbyStreets)
    cmb->addItem(street.c_str());
  cmb->setEditable(editable.m_address);
  cmb->setEnabled(editable.m_address);
  cmb->setObjectName(kStreetObjectName);
  streetRow->addWidget(cmb);
  vLayout->addLayout(streetRow);
  QHBoxLayout * houseRow = new QHBoxLayout();
  houseRow->addWidget(new QLabel(QString(kHouseNumberObjectName)));
  QLineEdit * houseLineEdit = new QLineEdit();
  houseLineEdit->setText(info.m_house.c_str());
  houseLineEdit->setReadOnly(!editable.m_address);
  houseLineEdit->setObjectName(kHouseNumberObjectName);
  houseRow->addWidget(houseLineEdit);
  vLayout->addLayout(houseRow);

  // All  metadata rows.
  QVBoxLayout * metaRows = new QVBoxLayout();
  for (Metadata::EType const field : editable.m_metadata)
  {
    QString const fieldName = QString::fromStdString(DebugPrint(field));
    QHBoxLayout * fieldRow = new QHBoxLayout();
    fieldRow->addWidget(new QLabel(fieldName));
    QLineEdit * lineEdit = new QLineEdit(QString::fromStdString(feature.GetMetadata().Get(field)));
    // Mark line editor to query it's text value when editing is finished.
    lineEdit->setObjectName(fieldName);
    fieldRow->addWidget(lineEdit);
    metaRows->addLayout(fieldRow);
  }
  vLayout->addLayout(metaRows);

  // Dialog buttons.
  QDialogButtonBox * buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Cancel | QDialogButtonBox::Save);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  // Delete button should send custom int return value from dialog.
  QPushButton * deletePOIButton = new QPushButton("Delete POI");
  QSignalMapper * signalMapper = new QSignalMapper();
  connect(deletePOIButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
  signalMapper->setMapping(deletePOIButton, QDialogButtonBox::DestructiveRole);
  connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(done(int)));
  buttonBox->addButton(deletePOIButton, QDialogButtonBox::DestructiveRole);
  QHBoxLayout * buttonsRowLayout = new QHBoxLayout();
  buttonsRowLayout->addWidget(buttonBox);
  vLayout->addLayout(buttonsRowLayout);

  setLayout(vLayout);
  setWindowTitle("POI Editor");
}

StringUtf8Multilang EditorDialog::GetEditedNames() const
{
  StringUtf8Multilang names;
  for (int8_t langCode = StringUtf8Multilang::DEFAULT_CODE; langCode < StringUtf8Multilang::MAX_SUPPORTED_LANGUAGES; ++langCode)
  {
    QLineEdit * le = findChild<QLineEdit *>(StringUtf8Multilang::GetLangByCode(langCode));
    if (!le)
      continue;
    string const name = le->text().toStdString();
    if (!name.empty())
      names.AddString(langCode, name);
  }
  return names;
}

Metadata EditorDialog::GetEditedMetadata() const
{
  Metadata metadata;
  for (int type = Metadata::FMD_CUISINE; type < Metadata::FMD_COUNT; ++type)
  {
    QLineEdit * editor = findChild<QLineEdit *>(QString::fromStdString(DebugPrint(static_cast<Metadata::EType>(type))));
    if (editor)
      metadata.Set(static_cast<Metadata::EType>(type), editor->text().toStdString());
  }
  return metadata;
}

string EditorDialog::GetEditedStreet() const
{
  QComboBox const * cmb = findChild<QComboBox *>();
  return cmb->currentText().toStdString();
}

string EditorDialog::GetEditedHouseNumber() const
{
  return findChild<QLineEdit *>(kHouseNumberObjectName)->text().toStdString();
}
