#include "searchwindow.hpp"

#include "../base/assert.hpp"
#include "../std/bind.hpp"
#include "../map/feature_vec_model.hpp"

#include <QtGui/QHeaderView>

#include "../base/start_mem_debug.hpp"


namespace qt {

FindTableWnd::FindTableWnd(QWidget * pParent, FindEditorWnd * pEditor, model_t * pModel)
: base_type(0, 1, pParent), m_pEditor(pEditor), m_pModel(pModel)
{
  horizontalHeader()->hide();
  verticalHeader()->hide();

  setShowGrid(false);
  setSelectionMode(QAbstractItemView::NoSelection);

  setMouseTracking(true);

  connect(m_pEditor, SIGNAL(textChanged(QString const &)), this, SLOT(OnTextChanged(QString const &)));
}

bool FindTableWnd::AddFeature(FeatureType const & f)
{
  string name = f.GetName();

  if (!name.empty())
  {
    // 200 rows is enough
    int const r = rowCount();
    if (r > 200)
      return false;

    insertRow(r);

    QTableWidgetItem * item = new QTableWidgetItem(QString::fromUtf8(name.c_str()));
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);
    setItem(r, 0, item);

    m_features.push_back(f);
  }

  return true;
}

void FindTableWnd::OnTextChanged(QString const & s)
{
  setRowCount(0);

  m_features.clear();

  if (!s.isEmpty())
  {
    QByteArray utf8bytes = s.toUtf8();
    // Search should be here, but it's currently unavailable.
  }
}

FeatureType const & FindTableWnd::GetFeature(size_t row) const
{
  ASSERT ( row < m_features.size(), (row, m_features.size()) );
  return m_features[row];
}

}
