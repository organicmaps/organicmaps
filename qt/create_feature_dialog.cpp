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

  auto const & categories = cats.GetAllCategoryNames(languages::GetCurrentNorm());
  for (auto const & entry : categories)
  {
    QListWidgetItem * lwi = new QListWidgetItem(entry.first.c_str() /* name */, allSortedList);
    lwi->setData(Qt::UserRole, entry.second /* type */);
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
