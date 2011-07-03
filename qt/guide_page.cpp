#include "guide_page.hpp"

#include "../words/sloynik_engine.hpp"

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

GuidePageHolder::~GuidePageHolder()
{
}

void GuidePageHolder::showEvent(QShowEvent * e)
{
  base_type::showEvent(e);

  m_pEditor->setFocus();
}

namespace
{
  sl::StrFn::Str const * StrCreate(char const * utf8Data, uint32_t size)
  {
    return reinterpret_cast<sl::StrFn::Str *>(new string(utf8Data, size));
  }

  void StrDestroy(sl::StrFn::Str const * s)
  {
    delete reinterpret_cast<string const *>(s);
  }

  int StrPrimaryCompare(void *, sl::StrFn::Str const * pa, sl::StrFn::Str const * pb)
  {
    string const & a = *reinterpret_cast<string const *>(pa);
    string const & b = *reinterpret_cast<string const *>(pb);
    return a == b ? 0 : (a < b ? -1 : 1);
  }

  int StrSecondaryCompare(void *, sl::StrFn::Str const * pa, sl::StrFn::Str const * pb)
  {
    string s1(*reinterpret_cast<string const *>(pa));
    string s2(*reinterpret_cast<string const *>(pb));
    std::use_facet<std::ctype<char> >(std::locale()).tolower(&s1[0], &s1[0] + s1.size());
    std::use_facet<std::ctype<char> >(std::locale()).tolower(&s2[0], &s2[0] + s2.size());
    return s1 == s2 ? 0 : (s1 < s2 ? -1 : 1);
  }
}

void GuidePageHolder::CreateEngine()
{
  sl::StrFn fn;
  fn.Create = &StrCreate;
  fn.Destroy = &StrDestroy;
  fn.PrimaryCompare = &StrPrimaryCompare;
  fn.SecondaryCompare = &StrSecondaryCompare;

  fn.m_pData = 0;
  fn.m_PrimaryCompareId = 1;
  fn.m_SecondaryCompareId = 2;

  m_pEngine.reset(new sl::SloynikEngine("dictionary.slf", "index", fn));
}

void GuidePageHolder::OnShowPage()
{
  sl::SloynikEngine::SearchResult res;
  string const prefix(m_pEditor->text().toUtf8().constData());
  m_pEngine->Search(prefix, res);

  sl::SloynikEngine::ArticleData data;
  m_pEngine->GetArticleData(res.m_FirstMatched, data);

  m_pView->setHtml(QString::fromUtf8(data.m_HTML.c_str()));
}

}
