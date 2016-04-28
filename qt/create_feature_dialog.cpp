#include "qt/create_feature_dialog.hpp"

#include "indexer/new_feature_categories.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QVBoxLayout>

CreateFeatureDialog::CreateFeatureDialog(QWidget * parent, osm::NewFeatureCategories const & cats)
  : QDialog(parent)
{
  // todo(@m) Fix this.
  /*
    QListWidget * lastUsedList = new QListWidget();

    for (auto const & cat : cats.m_lastUsed)
    {
      QListWidgetItem * lwi = new QListWidgetItem(cat.m_name.c_str(), lastUsedList);
      lwi->setData(Qt::UserRole, cat.m_type);
    }
    connect(lastUsedList, SIGNAL(clicked(QModelIndex const &)), this,
    SLOT(OnListItemSelected(QModelIndex const &)));

    QListWidget * allSortedList = new QListWidget();
    for (auto const & cat : cats.m_allSorted)
    {
      QListWidgetItem * lwi = new QListWidgetItem(cat.m_name.c_str(), allSortedList);
      lwi->setData(Qt::UserRole, cat.m_type);
    }
    connect(allSortedList, SIGNAL(clicked(QModelIndex const &)), this,
    SLOT(OnListItemSelected(QModelIndex const &)));

    QVBoxLayout * vBox = new QVBoxLayout();
    vBox->addWidget(lastUsedList);
    vBox->addWidget(allSortedList);


    QDialogButtonBox * dbb = new QDialogButtonBox();
    dbb->addButton(QDialogButtonBox::Close);
    connect(dbb, SIGNAL(clicked(QAbstractButton*)), this, SLOT(reject()));
    vBox->addWidget(dbb);

    setLayout(vBox);
    setWindowTitle("OSM Editor");
  */
}

void CreateFeatureDialog::OnListItemSelected(QModelIndex const & i)
{
  m_selectedType = static_cast<uint32_t>(i.data(Qt::UserRole).toULongLong());
  accept();
}
