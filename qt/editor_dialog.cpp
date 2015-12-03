#include "qt/editor_dialog.hpp"

#include "search/result.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_meta.hpp"

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
  // Second row: Name label and text input.
  QHBoxLayout * nameRow = new QHBoxLayout();
  nameRow->addWidget(new QLabel("Name:"));
  // TODO(AlexZ): Print names in all available languages.
  string defaultName, intName;
  feature.GetPreferredNames(defaultName, intName);
  QLineEdit * lineEditName = new QLineEdit(QString::fromStdString(defaultName));
  nameRow->addWidget(lineEditName);
  vLayout->addLayout(nameRow);

  // More rows: All  metadata rows.
  QVBoxLayout * metaRows = new QVBoxLayout();
  // TODO(mgsergio): Load editable fields from metadata.
  vector<Metadata::EType> editableMetadataFields;
  // TODO(AlexZ): Temporary enable only existing meta information fields.
  // Final editor should have all editable fields enabled.
  editableMetadataFields = feature.GetMetadata().GetPresentTypes();
/*
  // Merge editable fields for all feature's types.
  feature.ForEachType([&editableMetadataFields](uint32_t type)
  {
    auto const editableFields = osm::Editor::EditableMetadataForType(type);
    editableMetadataFields.insert(editableFields.begin(), editableFields.end());
  });
*/
  // Equals to editableMetadataFields, used to retrieve text entered by user.
  vector<QLineEdit *> metaFieldEditors;
  for (auto const field : editableMetadataFields)
  {
    QHBoxLayout * fieldRow = new QHBoxLayout();
    fieldRow->addWidget(new QLabel(QString::fromStdString(DebugPrint(field) + ":")));
    QLineEdit * lineEdit = new QLineEdit(QString::fromStdString(feature.GetMetadata().Get(field)));
    fieldRow->addWidget(lineEdit);
    metaFieldEditors.push_back(lineEdit);
    metaRows->addLayout(fieldRow);
  }
  ASSERT_EQUAL(editableMetadataFields.size(), metaFieldEditors.size(), ());
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
