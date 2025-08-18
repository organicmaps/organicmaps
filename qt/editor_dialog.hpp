#pragma once

#include <QtWidgets/QDialog>

namespace osm
{
class EditableMapObject;
}  // namespace osm

class EditorDialog : public QDialog
{
  Q_OBJECT

public:
  EditorDialog(QWidget * parent, osm::EditableMapObject & emo);
private slots:
  void OnSave();

private:
  osm::EditableMapObject & m_feature;
};
