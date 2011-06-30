#include "../base/SRC_FIRST.hpp"
#include "classificator_tree.hpp"

#include "../indexer/classificator_loader.hpp"
#include "../indexer/classificator.hpp"

#include "../platform/platform.hpp"

#include "../base/assert.hpp"

#include "../std/bind.hpp"

#include <QtGui/QTreeWidget>
#include <QtGui/QHBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QToolBar>
#include <QtGui/QFileDialog>

#include "../base/start_mem_debug.hpp"


namespace qt 
{

typedef ClassifObject::visible_mask_t mask_t;

///////////////////////////////////////////////////////////////////////////
// QClassifTree implementation
///////////////////////////////////////////////////////////////////////////

class QClassifTree : public QTreeWidget
{
  typedef QTreeWidget base_type;

  ClassifTreeHolder * get_parent()
  {
    return dynamic_cast<ClassifTreeHolder *>(parentWidget());
  }

public:
  QClassifTree(ClassifTreeHolder * pParent) : base_type(pParent) {}

protected:
  virtual void contextMenuEvent(QContextMenuEvent * /*e*/)
  {
    int const col = currentColumn();
    if (col == 1)
      get_parent()->EditItem(currentItem());
  }
};

///////////////////////////////////////////////////////////////////////////
// QEditChecks implementation
///////////////////////////////////////////////////////////////////////////

QEditChecks::QEditChecks(QWidget * pParent)
: base_type(pParent)
{
  QVBoxLayout * pLayout = new QVBoxLayout(this);

  for (size_t i = 0; i < s_count; ++i)
  {
    m_arr[i] = new QCheckBox(QString::number(i), this);
    pLayout->addWidget(m_arr[i]);
  }

  QPushButton * p = new QPushButton(tr("OK"), this);
  connect(p, SIGNAL(pressed()), this, SLOT(OnOK()));
  pLayout->addWidget(p);

  setLayout(pLayout);
}

void QEditChecks::Show(ClassifObject * p, QPoint const & pt)
{
  m_pObj = p;
  mask_t mask = p->GetVisibilityMask();

  for (size_t i = 0; i < s_count; ++i)
    m_arr[i]->setChecked(mask[i]);

  show();
  move(pt);
}

void QEditChecks::OnOK()
{
  mask_t mask;
  for (size_t i = 0; i < s_count; ++i)
    mask[i] = m_arr[i]->isChecked();

  close();

  m_pObj->SetVisibilityMask(mask);
  emit applied();
}

///////////////////////////////////////////////////////////////////////////
// QClassifTreeHolder implementation
///////////////////////////////////////////////////////////////////////////

ClassifTreeHolder::ClassifTreeHolder(QWidget * pParent,
                                     QWidget * drawWidget, char const * drawSlot)
: base_type(pParent)
{
  QVBoxLayout * pLayout = new QVBoxLayout(this);
  pLayout->setContentsMargins(0, 0, 0, 0);

  QToolBar * pToolBar = new QToolBar(this);
  pToolBar->setIconSize(QSize(32, 32));
  pToolBar->addAction(QIcon(":/classif32/save.png"), tr("Save visibility settings"), this, SLOT(OnSave()));
  pToolBar->addAction(QIcon(":/classif32/load.png"), tr("Load visibility settings"), this, SLOT(OnLoad()));
  pToolBar->addAction(QIcon(":/classif32/select.png"), tr("Select all checks"), this, SLOT(OnSelectAll()));
  pToolBar->addAction(QIcon(":/classif32/clear.png"), tr("Clear all checks"), this, SLOT(OnClearAll()));

  m_pTree = new QClassifTree(this);
  m_pEditor = new QEditChecks(this);

  connect(m_pEditor, SIGNAL(applied()), this, SLOT(OnEditFinished()));
  drawWidget->connect(m_pEditor, SIGNAL(applied()), drawWidget, drawSlot);
  drawWidget->connect(this, SIGNAL(redraw_model()), drawWidget, drawSlot);

  m_pTree->setColumnCount(2);

  QStringList headers;
  headers << tr("Type") << tr("Mask");
  m_pTree->setHeaderLabels(headers);

  pLayout->addWidget(pToolBar);
  pLayout->addWidget(m_pTree);
  setLayout(pLayout);
}

namespace 
{
  void to_item(QTreeWidgetItem * p, ClassifObject * pObj)
  {
    qulonglong ptr = reinterpret_cast<qulonglong>(pObj);
    return p->setData(1, Qt::UserRole, QVariant(ptr));
  }

  ClassifObject * from_item(QTreeWidgetItem * p)
  {
    bool isOK;
    qulonglong ptr = p->data(1, Qt::UserRole).toULongLong(&isOK);
    ASSERT ( isOK, () );
    return reinterpret_cast<ClassifObject *>(ptr);
  }
}

void ClassifTreeHolder::Process(QTreeWidgetItem * pParent, ClassifObject * p)
{
  QTreeWidgetItem * pItem = 0;

  // do not add root item (leave more useful space)
  if (p != m_pRoot)
  {
    QStringList values;
    values << QString::fromStdString(p->GetName()) << GetMaskValue(p);

    if (pParent)
      pItem = new QTreeWidgetItem(pParent, values);
    else
      pItem = new QTreeWidgetItem(m_pTree, values);

    to_item(pItem, p);
  }

  p->ForEachObject(bind(&ClassifTreeHolder::Process, this, pItem, _1));
}

QString ClassifTreeHolder::GetMaskValue(ClassifObject const * p) const
{
  mask_t mask = p->GetVisibilityMask();
  size_t const count = mask.size();
  string str;
  str.resize(count);
  for (size_t i = 0; i < mask.size(); ++i)
    str[i] = (mask[i] ? '1' : '0');

  return QString::fromStdString(str);
}

void ClassifTreeHolder::SetRoot(ClassifObject * pRoot)
{
  m_pTree->clear();

  m_pRoot = pRoot;
  Process(0, m_pRoot);
  m_pTree->expandAll();
}

void ClassifTreeHolder::EditItem(QTreeWidgetItem * p)
{
  m_pCurrent = p;
  ClassifObject * pObj = from_item(p);

  // find best position of edit-window newar the cursor
  QPoint pt = QCursor::pos();
  int const h = m_pEditor->frameSize().height();
  pt.ry() -= (h / 2);

  pt.ry() = max(0, pt.y());
  pt.ry() = min(mapToGlobal(rect().bottomRight()).y() - h, pt.y());

  // show window
  m_pEditor->Show(pObj, pt);
}

void ClassifTreeHolder::OnEditFinished()
{
  m_pCurrent->setText(1, GetMaskValue(from_item(m_pCurrent)));
}

void ClassifTreeHolder::OnSave()
{
  QString const fName = QFileDialog::getSaveFileName(this,
    tr("Save classificator visibility"),
    QString::fromStdString(GetPlatform().WritableDir()),
    tr("Text Files (*.txt)"));

  classif().PrintVisibility(fName.toAscii().constData());
}

void ClassifTreeHolder::OnLoad()
{
  QString const fName = QFileDialog::getOpenFileName(this,
    tr("Open classificator visibility"),
    QString::fromStdString(GetPlatform().WritableDir()),
    tr("Text Files (*.txt)"));

  classificator::ReadVisibility(fName.toAscii().constData());

  Rebuild();
}

namespace
{
  template <class ToDo> void ForEachRecursive(ClassifObject * p, ToDo & toDo)
  {
    toDo(p);
    p->ForEachObject(bind(&ForEachRecursive<ToDo>, _1, ref(toDo)));
  }

  class do_select
  {
    mask_t m_mask;
  public:
    do_select(mask_t mask) : m_mask(mask) {}
    void operator() (ClassifObject * p)
    {
      p->SetVisibilityMask(m_mask);
    }
  };
}

template <class TMask>
void ClassifTreeHolder::OnSetMask(TMask mask)
{
  do_select doSelect(mask);
  ForEachRecursive(GetRoot(), doSelect);
  Rebuild();
}

void ClassifTreeHolder::OnSelectAll()
{
  mask_t mask;
  mask.set();
  OnSetMask(mask);
}

void ClassifTreeHolder::OnClearAll()
{
  mask_t mask;
  mask.reset();
  OnSetMask(mask);
}

ClassifObject * ClassifTreeHolder::GetRoot()
{
  return classif().GetMutableRoot();
}

void ClassifTreeHolder::Rebuild()
{
  SetRoot(GetRoot());
  Redraw();
}

void ClassifTreeHolder::Redraw()
{
  emit redraw_model();
}

}
