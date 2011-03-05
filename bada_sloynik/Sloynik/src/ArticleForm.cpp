#include "ArticleForm.h"
#include "SearchForm.h"
#include <FApp.h>

using namespace Osp::Base;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

ArticleForm::ArticleForm() : m_pBuffer(NULL)
{
}

ArticleForm::~ArticleForm()
{
}

bool ArticleForm::Initialize()
{
  Construct(FORM_STYLE_NORMAL | FORM_STYLE_TITLE | FORM_STYLE_INDICATOR
        | FORM_STYLE_SOFTKEY_0);

  SetSoftkeyText(SOFTKEY_0, L"Search");
  SetSoftkeyActionId(SOFTKEY_0, ID_SEARCH_SOFTKEY);
  AddSoftkeyActionListener(SOFTKEY_0, *this);

  m_pWeb = new Osp::Web::Controls::Web();
  m_pWeb->Construct(Osp::Graphics::Rectangle(0, 0, 480, 700));
  AddControl(*m_pWeb);

  return true;
}

void ArticleForm::MyInit()
{
  SetTitleText(m_Name);
  m_pWeb->LoadData("", *m_pBuffer, "text/html", "UTF-8");
}

void ArticleForm::OnActionPerformed(const Osp::Ui::Control& source, int actionId)
{
  switch (actionId)
  {
    case ID_SEARCH_SOFTKEY:
    {
      Frame * pFrame =
          Osp::App::Application::GetInstance()->GetAppFrame()->GetFrame();
      pFrame->SetCurrentForm(*m_pSearchForm);
      m_pSearchForm->Draw();
      m_pSearchForm->Show();
    }
    break;
  }
}
