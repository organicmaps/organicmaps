#include "qlistmodel.hpp"

#include <QBrush>

QListModel::QListModel(QObject * parent, const QList<FontRange> & ranges)
  : base_t(parent)
  , m_ranges(ranges)
{
}

int QListModel::rowCount(const QModelIndex & /*parent*/) const
{
  return m_ranges.size();
}

int QListModel::columnCount(const QModelIndex & /*parent*/) const
{
  return 1;
}

QVariant QListModel::data(const QModelIndex & index, int role) const
{
  if (role == Qt::DisplayRole)
  {
    QString fontName = m_ranges[index.row()].m_fontPath;
    int pos = fontName.lastIndexOf("/");
    if (pos != -1)
      fontName = fontName.right(fontName.size() - pos - 1);
    return QString("%1 : 0x%2 - 0x%3").arg(fontName)
                                      .arg(m_ranges[index.row()].m_startRange, 0, 16)
                                      .arg(m_ranges[index.row()].m_endRange, 0, 16);

  }
  else if (role == Qt::BackgroundRole)
    return m_ranges[index.row()].m_validFont ? QBrush(Qt::white) : QBrush(Qt::red);

  return QVariant();
}

bool QListModel::setData(QModelIndex const & /*index*/, QVariant const & /*value*/, int /*role*/)
{
  return false;
}
