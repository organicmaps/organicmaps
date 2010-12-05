#pragma once

#include <QtGui/QWidget>
#include <QtGui/QDialog>

class ClassifObject;

class QTreeWidget;
class QTreeWidgetItem;
class QCheckBox;
class QToolBar;

namespace qt
{
  class QClassifTree;
  
  class QEditChecks : public QDialog
  {
    typedef QDialog base_type;

    static const int s_count = 18;
    QCheckBox * m_arr[s_count];

    ClassifObject * m_pObj;

    Q_OBJECT

  public:
    QEditChecks(QWidget * pParent);

    void Show(ClassifObject * p, QPoint const & cursor);

  public Q_SLOTS:
    void OnOK();

  Q_SIGNALS:
    void applied();
  };

  class ClassifTreeHolder : public QWidget
  {
    typedef QWidget base_type;

    void Process(QTreeWidgetItem * pParent, ClassifObject * p);

    QString GetMaskValue(ClassifObject const * p) const;

    Q_OBJECT

  public:
    ClassifTreeHolder(QWidget * pParent,
                      QWidget * drawWidget, char const * drawSlot);

    void SetRoot(ClassifObject * pRoot);

    void EditItem(QTreeWidgetItem * p);

  public Q_SLOTS:
    void OnEditFinished();
    void OnSave();
    void OnLoad();
    void OnSelectAll();
    void OnClearAll();

Q_SIGNALS:
    void redraw_model();

  protected:
    ClassifObject * GetRoot();
    void Rebuild();
    void Redraw();

  private:
    template <class TMask> void OnSetMask(TMask mask);

    QClassifTree * m_pTree;
    QEditChecks * m_pEditor;

    ClassifObject * m_pRoot;

    QTreeWidgetItem * m_pCurrent;
  };
}
