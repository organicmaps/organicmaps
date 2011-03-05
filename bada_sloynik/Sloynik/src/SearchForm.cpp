#include "SearchForm.h"
#include "ArticleForm.h"
#include "../../../words/sloynik_engine.hpp"
#include <FApp.h>
#include <locale>


using namespace Osp::Base;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

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

  int StrSecondaryCompare(void *,
      sl::StrFn::Str const * pa, sl::StrFn::Str const * pb)
  {
    string const & a = *reinterpret_cast<string const *>(pa);
    string const & b = *reinterpret_cast<string const *>(pb);
    return a == b ? 0 : (a < b ? -1 : 1);
  }

  int StrPrimaryCompare(void *,
      sl::StrFn::Str const * pa, sl::StrFn::Str const * pb)
  {
    string s1(*reinterpret_cast<string const *>(pa));
    string s2(*reinterpret_cast<string const *>(pb));
    std::use_facet<std::ctype<char> >(
        std::locale()).tolower(&s1[0], &s1[0] + s1.size());
    std::use_facet<std::ctype<char> >(
        std::locale()).tolower(&s2[0], &s2[0] + s2.size());
    return s1 == s2 ? 0 : (s1 < s2 ? -1 : 1);
  }
}


SearchForm::SearchForm(void) : m_pEngine(NULL), m_pArticleForm(NULL)
{
}

SearchForm::~SearchForm(void)
{
  // TODO: delete m_pEngine; delete m_pArticleForm;
}

bool SearchForm::Initialize()
{
	// Construct an XML form
	Construct(L"IDF_SEARCHFORM");

	return true;
}

result SearchForm::OnInitializing(void)
{
	result r = E_SUCCESS;

	// TODO: Add your initialization code here

  string const dictionaryPath = "/Home/wordnet.slf";
  string const indexPath = "/Home/index";
  string const tempPath = "/Home/index_tmp";
  vector<pair<string, uint64_t> > dictionaries;
  dictionaries.push_back(make_pair(dictionaryPath, 1ULL));
  sl::StrFn strFn;
  strFn.Create = StrCreate;
  strFn.Destroy = StrDestroy;
  strFn.PrimaryCompare = StrPrimaryCompare;
  strFn.SecondaryCompare = StrSecondaryCompare;
  strFn.m_pData = NULL;
  strFn.m_PrimaryCompareId = 1;
  strFn.m_SecondaryCompareId = 2;
  m_pEngine = new sl::SloynikEngine(indexPath, tempPath, strFn, dictionaries);


	m_pCustomListItemFormat = new CustomListItemFormat();
  m_pCustomListItemFormat->Construct();
  m_pCustomListItemFormat->AddElement(TEXT_ID,
      Osp::Graphics::Rectangle(0, 0, GetWidth(), ITEM_HEIGHT));


	m_pSearchField =
	    static_cast<EditField *>(GetControl(L"IDPC_SEARCH_EDIT", true));
	m_pResultsList =
	    static_cast<SlidableList *>(GetControl(L"IDPC_RESULTS_LIST", true));

	m_pSearchField->AddTextEventListener(*this);
	m_pSearchField->AddActionEventListener(*this);
	m_pResultsList->AddSlidableListEventListener(*this);
	m_pResultsList->AddCustomItemEventListener(*this);

	m_pSearchField->SetOverlayKeypadCommandButton(
	    COMMAND_BUTTON_POSITION_LEFT, L"Clear", ID_CLEAR_SEARCH_FIELD);


	// m_pResultsList->AddSlidableListEventListener(*this);
	/*
	// Get a button via resource ID
	__pButtonOk = static_cast<Button *>(GetControl(L"IDC_BUTTON_OK"));
	if (__pButtonOk != null)
	{
		__pButtonOk->SetActionId(ID_BUTTON_OK);
		__pButtonOk->AddActionEventListener(*this);
	}
	*/

	return r;
}

result
SearchForm::OnTerminating(void)
{
	result r = E_SUCCESS;

	// TODO: Add your termination code here

	return r;
}

void
SearchForm::OnActionPerformed(const Osp::Ui::Control& source, int actionId)
{
	switch(actionId)
	{
	case ID_CLEAR_SEARCH_FIELD:
		{
			m_pSearchField->SetText(L"");
			m_pResultsList->ScrollToTop();
			m_pResultsList->RequestRedraw();
		}
		break;
	default:
		break;
	}
}

void SearchForm::OnTextValueChanged(Osp::Ui::Control const &)
{
  sl::SloynikEngine::SearchResult searchResult;
  String text = m_pSearchField->GetText();
  ByteBuffer * buf = Utility::StringUtil::StringToUtf8N(text);
  if (buf)
  {
    char const * utf8Str = reinterpret_cast<char const *>(buf->GetPointer());
    m_pEngine->Search(utf8Str, searchResult);
    delete buf;
    m_pResultsList->ScrollToTop(searchResult.m_FirstMatched);
    m_pResultsList->RequestRedraw();
  }
}

void SearchForm::OnTextValueChangeCanceled(Osp::Ui::Control const &)
{
}

void SearchForm::OnListPropertyRequested(const Osp::Ui::Control&)
{
  uint32_t const itemCount = m_pEngine ? m_pEngine->WordCount() : 0;
  m_pResultsList->SetItemCountAndHeight(itemCount, itemCount * ITEM_HEIGHT);
}

void SearchForm::OnLoadToTopRequested(
    const Osp::Ui::Control&, int index, int numItems)
{
  for (int i = index; i > index - numItems; i--)
    m_pResultsList->LoadItemToTop(*CreateListItem(i), i + 1);
}

void SearchForm::OnLoadToBottomRequested(
    const Osp::Ui::Control&, int index, int numItems)
{
  for (int i = index; i < index + numItems; i++)
    m_pResultsList->LoadItemToBottom(*CreateListItem(i), i + 1);
}

void SearchForm::OnUnloadItemRequested(const Osp::Ui::Control&, int)
{
  // TODO: OnUnloadItemRequested
}

CustomListItem * SearchForm::CreateListItem(uint32_t id)
{
  sl::SloynikEngine::WordInfo info;
  m_pEngine->GetWordInfo(id, info);
  String text;
  Utility::StringUtil::Utf8ToString(info.m_Word.c_str(), text);

  CustomListItem * pItem = new CustomListItem();
  pItem->Construct(ITEM_HEIGHT);
  pItem->SetItemFormat(*m_pCustomListItemFormat);
  pItem->SetElement(TEXT_ID, text);
  return pItem;
}

void SearchForm::OnItemStateChanged(const Osp::Ui::Control & source, int index,
    int itemId, int /*elementId*/, Osp::Ui::ItemStatus status)
{
  this->OnItemStateChanged(source, index, itemId, status);
}

void SearchForm::OnItemStateChanged(const Osp::Ui::Control &source, int index,
    int itemId, Osp::Ui::ItemStatus status)
{
  Frame * pFrame =
      Osp::App::Application::GetInstance()->GetAppFrame()->GetFrame();
  if (!m_pArticleForm)
  {
    m_pArticleForm = new ArticleForm();
    m_pArticleForm->m_pSearchForm = this;
    m_pArticleForm->Initialize();
    pFrame->AddControl(*m_pArticleForm);
  }

  sl::SloynikEngine::WordInfo info;
  m_pEngine->GetWordInfo(index, info);
  Utility::StringUtil::Utf8ToString(info.m_Word.c_str(), m_pArticleForm->m_Name);

  sl::SloynikEngine::ArticleData data;
  m_pEngine->GetArticleData(index, data);
  data.m_HTML.push_back(0);
  delete m_pArticleForm->m_pBuffer;
  m_pArticleForm->m_pBuffer = new ByteBuffer;
  m_pArticleForm->m_pBuffer->Construct(data.m_HTML.size());
  m_pArticleForm->m_pBuffer->SetArray(
      reinterpret_cast<byte const *>(&data.m_HTML[0]), 0, data.m_HTML.size());

  m_pArticleForm->MyInit();
  pFrame->SetCurrentForm(*m_pArticleForm);
  m_pArticleForm->Draw();
  m_pArticleForm->Show();
}
