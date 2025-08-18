#pragma once

#include <QtWidgets/QDialog>

class QModelIndex;
namespace osm
{
class NewFeatureCategories;
}  // namespace osm

class CreateFeatureDialog : public QDialog
{
  Q_OBJECT

public:
  CreateFeatureDialog(QWidget * parent, osm::NewFeatureCategories & cats);
  /// Valid only if dialog has finished with Accepted code.
  uint32_t GetSelectedType() const { return m_selectedType; }

private slots:
  void OnListItemSelected(QModelIndex const & i);

private:
  uint32_t m_selectedType = 0;
};
