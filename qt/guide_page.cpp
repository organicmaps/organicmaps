#include "guide_page.hpp"

#include "../platform/platform.hpp"

#include "../coding/strutil.hpp"

#include <QtWebKit/QWebView>

#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>


namespace qt {

GuidePageHolder::GuidePageHolder(QWidget * pParent)
  : base_type(pParent)
{
  QVBoxLayout * pLayout = new QVBoxLayout(this);
  pLayout->setContentsMargins(0, 0, 0, 0);

  m_pEditor = new QLineEdit(this);

  m_pView = new QWebView(this);

  connect(m_pEditor, SIGNAL(returnPressed()), this, SLOT(OnShowPage()));

  pLayout->addWidget(m_pEditor);
  pLayout->addWidget(m_pView);
  setLayout(pLayout);

  CreateEngine();
}

void GuidePageHolder::showEvent(QShowEvent * e)
{
  base_type::showEvent(e);

  m_pEditor->setFocus();
}

namespace
{
  sl::StrFn::Str const * StrCreate(char const * pUtf8Data, uint32_t sz)
  {
    wstring * s = new wstring();
    *s = FromUtf8(string(pUtf8Data));
    return reinterpret_cast<sl::StrFn::Str const *>(s);
  }

  void StrDestroy(sl::StrFn::Str const * p)
  {
    delete reinterpret_cast<wstring const *>(p);
  }

  int StrPrimaryCompare(void *, sl::StrFn::Str const * a, sl::StrFn::Str const * b)
  {
    wstring const * pA = reinterpret_cast<wstring const *>(a);
    wstring const * pB = reinterpret_cast<wstring const *>(b);
    return *pA == *pB;
  }

  int StrSecondaryCompare(void *, sl::StrFn::Str const * a, sl::StrFn::Str const * b)
  {
    wstring const * pA = reinterpret_cast<wstring const *>(a);
    wstring const * pB = reinterpret_cast<wstring const *>(b);
    return *pA == *pB;
  }
}

void GuidePageHolder::CreateEngine()
{
  string const dicPath = GetPlatform().ReadPathForFile("dictionary.slf");
  string const indPath = GetPlatform().WritableDir() + "index";

  sl::StrFn fn;
  fn.Create = &StrCreate;
  fn.Destroy = &StrDestroy;
  fn.PrimaryCompare = &StrPrimaryCompare;
  fn.SecondaryCompare = &StrSecondaryCompare;

  fn.m_pData = 0;
  fn.m_PrimaryCompareId = 1;
  fn.m_SecondaryCompareId = 2;

  m_pEngine.reset(new sl::SloynikEngine(dicPath, indPath, fn));
}

void GuidePageHolder::OnShowPage()
{
  sl::SloynikEngine::SearchResult res;
  m_pEngine->Search(m_pEditor->text().toStdString(), res);

  sl::SloynikEngine::ArticleData data;
  m_pEngine->GetArticleData(res.m_FirstMatched, data);

  m_pView->setHtml(QString::fromUtf8(data.m_HTML.c_str()));
}

}
