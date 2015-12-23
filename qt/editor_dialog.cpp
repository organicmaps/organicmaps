#include "qt/editor_dialog.hpp"

#include "search/result.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/osm_editor.hpp"

#include "base/collection_cast.hpp"

#include "std/set.hpp"
#include "std/vector.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <QtCore/QSignalMapper>

using feature::Metadata;

EditorDialog::EditorDialog(QWidget * parent, FeatureType const & feature) : QDialog(parent)
{
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

  // Rows block: Name(s) label(s) and text input.
  char const * defaultLangStr = StringUtf8Multilang::GetLangByCode(StringUtf8Multilang::DEFAULT_CODE);
  // Default name editor is always displayed, even if feature name is empty.
  QHBoxLayout * defaultNameRow = new QHBoxLayout();
  defaultNameRow->addWidget(new QLabel(QString("Name:")));
  QLineEdit * defaultNamelineEdit = new QLineEdit();
  defaultNamelineEdit->setObjectName(defaultLangStr);
  defaultNameRow->addWidget(defaultNamelineEdit);
  vLayout->addLayout(defaultNameRow);

  feature.ForEachNameRef([&vLayout, &defaultNamelineEdit](int8_t langCode, string const & name) -> bool
  {
    if (langCode == StringUtf8Multilang::DEFAULT_CODE)
      defaultNamelineEdit->setText(QString::fromStdString(name));
    else
    {
      QHBoxLayout * nameRow = new QHBoxLayout();
      char const * langStr = StringUtf8Multilang::GetLangByCode(langCode);
      nameRow->addWidget(new QLabel(QString("Name:") + langStr));
      QLineEdit * lineEditName = new QLineEdit(QString::fromStdString(name));
      lineEditName->setObjectName(langStr);
      nameRow->addWidget(lineEditName);
      vLayout->addLayout(nameRow);
    }
    return true; // true is needed to enumerate all languages.
  });

  // All  metadata rows.
  QVBoxLayout * metaRows = new QVBoxLayout();
  // TODO(mgsergio): Load editable fields from metadata. Features can have several types, so we merge all editable fields here.
  set<Metadata::EType> const  editableMetadataFields =
      my::collection_cast<set>(osm::Editor::Instance().EditableMetadataForType(feature));

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
