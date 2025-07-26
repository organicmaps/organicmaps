#include "qt/create_feature_dialog.hpp"

#include "editor/new_feature_categories.hpp"

#include "indexer/classificator.hpp"

#include "platform/preferred_languages.hpp"

#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QVBoxLayout>

CreateFeatureDialog::CreateFeatureDialog(QWidget * parent, osm::NewFeatureCategories & cats) : QDialog(parent)
{
  cats.AddLanguage("en");
  cats.AddLanguage(languages::GetCurrentNorm());

  QListWidget * allSortedList = new QListWidget();

  for (auto const & name : cats.GetAllCreatableTypeNames())
    new QListWidgetItem(QString::fromStdString(name), allSortedList);

  connect(allSortedList, &QAbstractItemView::clicked, this, &CreateFeatureDialog::OnListItemSelected);

  QDialogButtonBox * dbb = new QDialogButtonBox();
  dbb->addButton(QDialogButtonBox::Close);
  connect(dbb, &QDialogButtonBox::clicked, this, &QDialog::reject);

  QVBoxLayout * vBox = new QVBoxLayout();
  vBox->addWidget(allSortedList);
  vBox->addWidget(dbb);
  setLayout(vBox);

  setWindowTitle("OSM Editor");
}

void CreateFeatureDialog::OnListItemSelected(QModelIndex const & i)
{
  auto const clType = i.data(Qt::DisplayRole).toString().toStdString();
  m_selectedType = classif().GetTypeByReadableObjectName(clType);
  ASSERT(m_selectedType != ftype::GetEmptyValue(), ());

  accept();
}
