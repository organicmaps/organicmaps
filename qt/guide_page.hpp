#pragma once

#include "../std/scoped_ptr.hpp"

#include <QtGui/QWidget>


class QLineEdit;
class QWebView;

namespace sl { class SloynikEngine; }

namespace qt
{
  class GuidePageHolder : public QWidget
  {
    typedef QWidget base_type;

    Q_OBJECT;

  public:
    GuidePageHolder(QWidget * pParent);
    virtual ~GuidePageHolder();

  protected:
    void CreateEngine();

    virtual void showEvent(QShowEvent * e);

  protected Q_SLOTS:
    void OnShowPage();

  private:
    QLineEdit * m_pEditor;
    QWebView * m_pView;

    scoped_ptr<sl::SloynikEngine> m_pEngine;
  };
}
