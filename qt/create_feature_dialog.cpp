#include "qt/create_feature_dialog.hpp"

#include "platform/preferred_languages.hpp"

#include "editor/new_feature_categories.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QVBoxLayout>

CreateFeatureDialog::CreateFeatureDialog(QWidget * parent, osm::NewFeatureCategories & cats)
  : QDialog(parent)
{
  cats.AddLanguage("en");  // Default.
  cats.AddLanguage(languages::GetCurrentNorm());

  QListWidget * allSortedList = new QListWidget();

  auto const & typeNames = cats.GetAllCreatableTypeNames();
  for (auto const & name : typeNames)
  {
    new QListWidgetItem(name.c_str() /* name */, allSortedList);
  }
  connect(allSortedList, SIGNAL(clicked(QModelIndex const &)), this,
          SLOT(OnListItemSelected(QModelIndex const &)));

  QDialogButtonBox * dbb = new QDialogButtonBox();
  dbb->addButton(QDialogButtonBox::Close);
  connect(dbb, SIGNAL(clicked(QAbstractButton *)), this, SLOT(reject()));

  QVBoxLayout * vBox = new QVBoxLayout();
  vBox->addWidget(allSortedList);
  vBox->addWidget(dbb);
  setLayout(vBox);

  setWindowTitle("OSM Editor");
}

void CreateFeatureDialog::OnListItemSelected(QModelIndex const & i)
{
  m_selectedType = static_cast<uint32_t>(i.data(Qt::UserRole).toULongLong());
  accept();
}
