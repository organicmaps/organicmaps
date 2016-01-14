#include "qt/editor_dialog.hpp"

#include "map/framework.hpp"

#include "search/result.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
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

constexpr char const * kStreetObjectName = "street";
constexpr char const * kHouseNumberObjectName = "houseNumber";

EditorDialog::EditorDialog(QWidget * parent, FeatureType const & feature, Framework & frm) : QDialog(parent)
{
  osm::Editor & editor = osm::Editor::Instance();

  QVBoxLayout * vLayout = new QVBoxLayout();

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

  bool const readOnlyName = !editor.IsNameEditable(feature);
  // Rows block: Name(s) label(s) and text input.
  char const * defaultLangStr = StringUtf8Multilang::GetLangByCode(StringUtf8Multilang::DEFAULT_CODE);
  // Default name editor is always displayed, even if feature name is empty.
  QHBoxLayout * defaultNameRow = new QHBoxLayout();
  defaultNameRow->addWidget(new QLabel(QString("Name:")));
  QLineEdit * defaultNamelineEdit = new QLineEdit();
  defaultNamelineEdit->setReadOnly(readOnlyName);
  defaultNamelineEdit->setObjectName(defaultLangStr);
  defaultNameRow->addWidget(defaultNamelineEdit);
  vLayout->addLayout(defaultNameRow);

  feature.ForEachNameRef([&](int8_t langCode, string const & name) -> bool
  {
    if (langCode == StringUtf8Multilang::DEFAULT_CODE)
      defaultNamelineEdit->setText(QString::fromStdString(name));
    else
    {
      QHBoxLayout * nameRow = new QHBoxLayout();
      char const * langStr = StringUtf8Multilang::GetLangByCode(langCode);
      nameRow->addWidget(new QLabel(QString("Name:") + langStr));
      QLineEdit * lineEditName = new QLineEdit(QString::fromStdString(name));
      lineEditName->setReadOnly(readOnlyName);
      lineEditName->setObjectName(langStr);
      nameRow->addWidget(lineEditName);
      vLayout->addLayout(nameRow);
    }
    return true; // true is needed to enumerate all languages.
  });

  // Address rows.
  bool const readOnlyAddress = !editor.IsAddressEditable(feature);
  vector<string> nearbyStreets = frm.GetNearbyFeatureStreets(feature);
  // If feature does not have a specified street, display empty combo box.
  search::AddressInfo const info = frm.GetFeatureAddressInfo(feature);
  if (info.m_street.empty())
    nearbyStreets.insert(nearbyStreets.begin(), "");
  QHBoxLayout * streetRow = new QHBoxLayout();
  streetRow->addWidget(new QLabel(QString("Street:")));
  QComboBox * cmb = new QComboBox();
  for (auto const & street : nearbyStreets)
    cmb->addItem(street.c_str());
  cmb->setEditable(!readOnlyAddress);
  cmb->setEnabled(!readOnlyAddress);
  cmb->setObjectName(kStreetObjectName);
  streetRow->addWidget(cmb) ;
  vLayout->addLayout(streetRow);
  QHBoxLayout * houseRow = new QHBoxLayout();
  houseRow->addWidget(new QLabel(QString("House Number:")));
  QLineEdit * houseLineEdit = new QLineEdit();
  houseLineEdit->setText(info.m_house.c_str());
  houseLineEdit->setReadOnly(readOnlyAddress);
  houseLineEdit->setObjectName(kHouseNumberObjectName);
  houseRow->addWidget(houseLineEdit);
  vLayout->addLayout(houseRow);

  // All  metadata rows.
  QVBoxLayout * metaRows = new QVBoxLayout();
  vector<Metadata::EType> const editableMetadataFields = editor.EditableMetadataForType(feature);
  for (Metadata::EType const field : editableMetadataFields)
  {
    QString const fieldName = QString::fromStdString(DebugPrint(field));
    QHBoxLayout * fieldRow = new QHBoxLayout();
    fieldRow->addWidget(new QLabel(fieldName + ":"));
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
  if (cmb->count())
    return cmb->itemText(0).toStdString();
  return string();
}

string EditorDialog::GetEditedHouseNumber() const
{
  return findChild<QLineEdit *>(kHouseNumberObjectName)->text().toStdString();
}
