#include "SearchForm.hpp"
#include "SceneRegister.hpp"
#include "MapsWithMeForm.hpp"
#include "AppResourceId.h"

#include "../../../map/user_mark.hpp"
#include "../../../map/framework.hpp"
#include "../../../search/result.hpp"
#include "../../../platform/settings.hpp"
#include "../../../platform/tizen_utils.hpp"
#include "../../../base/logging.hpp"

#include <FWeb.h>
#include <FAppApp.h>
#include <FApp.h>
#include "Utils.hpp"
#include "Framework.hpp"

using namespace Tizen::Base;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;
using namespace Tizen::App;
using namespace Tizen::Web::Controls;
using namespace Tizen::Graphics;
using namespace search;

namespace detail
{
typedef vector<String> CategoriesT;
CategoriesT const & GetCategories()
{
  static CategoriesT vr;
  if (vr.empty())
  {
    vr.push_back(GetString(IDS_FOOD));
    vr.push_back(GetString(IDS_SHOP));
    vr.push_back(GetString(IDS_HOTEL));
    vr.push_back(GetString(IDS_TOURISM));
    vr.push_back(GetString(IDS_ENTERTAINMENT));
    vr.push_back(GetString(IDS_ATM));
    vr.push_back(GetString(IDS_BANK));
    vr.push_back(GetString(IDS_TRANSPORT));
    vr.push_back(GetString(IDS_FUEL));
    vr.push_back(GetString(IDS_PARKING));
    vr.push_back(GetString(IDS_PHARMACY));
    vr.push_back(GetString(IDS_HOSPITAL));
    vr.push_back(GetString(IDS_TOILET));
    vr.push_back(GetString(IDS_POST));
    vr.push_back(GetString(IDS_POLICE));
  }
  return vr;
}

::Framework * GetFramework()
{
  return ::tizen::Framework::GetInstance();
}

static Color const white(0xFF,0xFF,0xFF);
static Color const gray(0xB0,0xB0,0xB0);

static int topHght = 27; //margin from top to text
static int btwWdth = 20; //margin between texts
static int imgWdth = 60; //left img width
static int imgHght = 60; //left img height
static int lstItmHght = 120; //list item height
static int backWdth = 150; //back txt width
static int mainFontSz = 45; //big font
static int minorFontSz = 25; //small font

CustomItem * CreateFeatureItem(Result const & val, double itemWidth)
{
  String itemText = val.GetString();
  CustomItem * pItem = new CustomItem();

  pItem->Construct(FloatDimension(itemWidth, lstItmHght), LIST_ANNEX_STYLE_NORMAL);
  FloatRectangle imgRect(btwWdth, topHght, imgWdth, imgHght);
  pItem->AddElement(imgRect, 0, *GetBitmap(IDB_SINGLE_RESULT), null, null);
  int txtWdht = itemWidth - btwWdth - imgWdth - backWdth;
  pItem->AddElement(FloatRectangle(btwWdth + imgWdth + btwWdth, 15, txtWdht, imgHght), 1, val.GetString(), mainFontSz, white, white, white);
  pItem->AddElement(FloatRectangle(btwWdth + imgWdth + btwWdth, 60.0f, txtWdht, imgHght), 2, val.GetRegionString(), minorFontSz, white, white, white);
  String feature = val.GetFeatureType();
  int ind;
  if (feature.IndexOf(" ", 0, ind) == E_SUCCESS)
  {
    String s;
    feature.SubString(0,ind,s);
    feature = s;
  }
  pItem->AddElement(FloatRectangle(itemWidth - backWdth, 10, backWdth, imgHght), 3, feature, minorFontSz, gray, gray, gray);
  double lat, lon;
  GetFramework()->GetCurrentPosition(lat, lon);
  double north = 0;
  string distance;
  double azimut;
  GetFramework()->GetDistanceAndAzimut(val.GetFeatureCenter(), lat, lon, north, distance, azimut);
  String dist(distance.c_str());
  pItem->AddElement(FloatRectangle(itemWidth - backWdth, 50, backWdth, imgHght), 4, dist, minorFontSz, gray, gray, gray);

  return pItem;
}

CustomItem * CreateSuggestionItem(String const & val, double itemWidth)
{
  String itemText = val;
  CustomItem * pItem = new CustomItem();

  pItem->Construct(FloatDimension(itemWidth, lstItmHght), LIST_ANNEX_STYLE_NORMAL);
  FloatRectangle imgRect(btwWdth, topHght, imgWdth, imgHght);
  pItem->AddElement(imgRect, 0, *GetBitmap(IDB_SUGGESTION_RESULT), null, null);
  pItem->AddElement(FloatRectangle(btwWdth + imgWdth + btwWdth, topHght, itemWidth, imgHght), 1, itemText, mainFontSz, white, white, white);

  return pItem;
}

} // detail

using namespace detail;


SearchForm::SearchForm()
:m_searchBar(0)
{
}

SearchForm::~SearchForm(void)
{
}

bool SearchForm::Initialize(void)
{
  Construct(IDF_SEARCH_FORM);
  return true;
}

result SearchForm::OnInitializing(void)
{
  m_searchBar = static_cast<SearchBar *>(GetControl(IDC_SEARCHBAR, true));
  m_searchBar->SetMode(SEARCH_BAR_MODE_INPUT);
  m_searchBar->AddActionEventListener(*this);
  m_searchBar->AddTextEventListener(*this);

  ListView * pList = static_cast<ListView *>(GetControl(IDC_LISTVIEW, true));
  pList->SetItemProvider(*this);
  pList->AddListViewItemEventListener(*this);
  pList->AddScrollEventListener(*this);

  SetFormBackEventListener(this);

  return E_SUCCESS;
}

void SearchForm::OnActionPerformed(Tizen::Ui::Control const & source, int actionId)
{
  if (actionId == m_searchBar->GetButtonActionId())
  {
    SceneManager * pSceneManager = SceneManager::GetInstance();
    pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
  }
}

void SearchForm::OnTextValueChanged(Tizen::Ui::Control const & source)
{
  Search(GetSearchString());
}

void SearchForm::OnSearchResultsReceived(search::Results const & results)
{
  if (results.IsEndMarker())
  {
    if (!results.IsEndedNormal())
      m_curResults.Clear();
    UpdateList();
  }
  else
    m_curResults = results;
}

void SearchForm::OnFormBackRequested(Tizen::Ui::Controls::Form & source)
{
  SceneManager * pSceneManager = SceneManager::GetInstance();
  pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
}

ListItemBase * SearchForm::CreateItem (int index, float itemWidth)
{
  if(IsShowCategories())
  {
    return CreateSuggestionItem(GetCategories()[index], itemWidth);
  }
  else
  {
    if (m_curResults.GetCount() == 0)
    {
      String itemText = GetString(IDS_NO_SEARCH_RESULTS_FOUND);
      CustomItem * pItem = new CustomItem();
      pItem->Construct(FloatDimension(itemWidth, lstItmHght), LIST_ANNEX_STYLE_NORMAL);
      pItem->AddElement(FloatRectangle(btwWdth, topHght, itemWidth, imgHght), 0, itemText, mainFontSz, white, white, white);
      return pItem;
    }
    else
    {
      Result const & res = m_curResults.GetResult(index);
      if (res.GetResultType() == Result::RESULT_SUGGESTION)
        return CreateSuggestionItem(res.GetString(), itemWidth);
      else
        return CreateFeatureItem(res, itemWidth);
    }
  }
}

void SearchForm::OnListViewItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListItemStatus status)
{
  if(IsShowCategories())
  {
    m_searchBar->SetText(GetCategories()[index]);
    Search(GetSearchString());
    Invalidate(true);
  }
  else
  {
    if (m_curResults.GetCount() > 0)
    {
      Result res = m_curResults.GetResult(index);
      if (res.GetResultType() == Result::RESULT_SUGGESTION)
      {
        m_searchBar->SetText(res.GetString());
        Search(GetSearchString());
        Invalidate(true);
      }
      else
      {
        GetFramework()->ShowSearchResult(res);
        SceneManager * pSceneManager = SceneManager::GetInstance();
        pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
      }
    }
  }
}

void SearchForm::OnScrollPositionChanged(Tizen::Ui::Control & source, int scrollPosition)
{
  m_searchBar->HideKeypad();
}

bool SearchForm::DeleteItem (int index, Tizen::Ui::Controls::ListItemBase * pItem, float itemWidth)
{
  delete pItem;
  return true;
}

int SearchForm::GetItemCount(void)
{
  if (IsShowCategories())
  {
    return GetCategories().size();
  }
  else
  {
    if (m_curResults.GetCount() == 0)
      return 1;
    else
      return m_curResults.GetCount();
  }
}

void SearchForm::UpdateList()
{
  ListView * pList = static_cast<ListView *>(GetControl(IDC_LISTVIEW));
  pList->UpdateList();
  pList->RemoveScrollEventListener(*this);
  pList->ScrollToItem(0);
  pList->AddScrollEventListener(*this);
}

void SearchForm::Search(String const & val)
{
  search::SearchParams m_params;
  m_params.m_callback = bind(&SearchForm::OnSearchResultsReceived, this, _1);
  m_params.m_query = FromTizenString(val);
  double lat, lon;
  GetFramework()->GetCurrentPosition(lat, lon);
  m_params.SetPosition(lat, lon);

  GetFramework()->Search(m_params);
}

String SearchForm::GetSearchString() const
{
  return m_searchBar->GetText();
}

bool SearchForm::IsShowCategories() const
{
  return GetSearchString().IsEmpty();
}
