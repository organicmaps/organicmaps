#pragma once

#include "engine.hpp"
#include <QAbstractListModel>

class QListModel : public QAbstractListModel
{
  typedef QAbstractListModel base_t;

public:
  QListModel(QObject * parent, QList<FontRange> const & ranges);

  int rowCount(const QModelIndex & parent) const;
  int columnCount(const QModelIndex & parent) const;
  QVariant data(const QModelIndex & index, int role) const;
  bool setData(const QModelIndex & index, const QVariant & value, int role);

private:
  QList<FontRange> m_ranges;
};
