#pragma once

#include <QtWidgets/QStyledItemDelegate>

class QAbstractItemModel;
class QComboBox;
class QTableView;
class QWidget;

namespace openlr
{
class ComboBoxDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:
  ComboBoxDelegate(QObject * parent = 0);

  QWidget * createEditor(QWidget * parent, QStyleOptionViewItem const & option,
                         QModelIndex const & index) const Q_DECL_OVERRIDE;

  void setEditorData(QWidget * editor, QModelIndex const & index) const Q_DECL_OVERRIDE;

  void setModelData(QWidget * editor, QAbstractItemModel * model, QModelIndex const & index) const Q_DECL_OVERRIDE;

  void updateEditorGeometry(QWidget * editor, QStyleOptionViewItem const & option,
                            QModelIndex const & index) const Q_DECL_OVERRIDE;
};

class TrafficPanel : public QWidget
{
  Q_OBJECT

public:
  explicit TrafficPanel(QAbstractItemModel * trafficModel, QWidget * parent);

private:
  void CreateTable(QAbstractItemModel * trafficModel);
  void FillTable();

signals:

public slots:
  // void OnCheckBoxClicked(int row, int state);

private:
  QTableView * m_table = Q_NULLPTR;
};
}  // namespace openlr
