#include "CategoryForm.hpp"
#include "SceneRegister.hpp"
#include "MapsWithMeForm.hpp"
#include "AppResourceId.h"
#include "Utils.hpp"
#include "Framework.hpp"
#include "Constants.hpp"
#include "BookMarkUtils.hpp"
#include "../../../map/framework.hpp"
#include "../../../platform/settings.hpp"
#include "../../../platform/tizen_utils.hpp"
#include "../../../base/logging.hpp"

using namespace Tizen::Base;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;
using namespace Tizen::Graphics;
using namespace consts;
using namespace bookmark;

CategoryForm::CategoryForm()
{
}

CategoryForm::~CategoryForm(void)
{
}

bool CategoryForm::Initialize(void)
{
  Construct(IDF_CATEGORY_FORM);
  return true;
}

result CategoryForm::OnInitializing(void)
{
  m_bEditState = false;
  m_itemToDelete = -1;
  m_curCategory = -1;

  ListView * pList = static_cast<ListView *>(GetControl(IDC_LISTVIEW, true));
  pList->SetItemProvider(*this);
  pList->AddListViewItemEventListener(*this);
  GetHeader()->AddActionEventListener(*this);

  EditField * pEditName = static_cast<EditField *>(GetControl(IDC_EDITFIELD_NAME, true));
  pEditName->AddTextEventListener(*this);

  SetFormBackEventListener(this);
  UpdateState();
  return E_SUCCESS;
}

void CategoryForm::OnActionPerformed(Tizen::Ui::Control const & source, int actionId)
{
  BookMarkManager & mngr = GetBMManager();
  switch(actionId)
  {
    case ID_EDIT:
    {
      m_bEditState = !m_bEditState;
      m_itemToDelete = -1;
      UpdateState();
      break;
    }
    case ID_VISIBILITY_ON:
    {
      mngr.SetCategoryVisible(m_curCategory, true);
      break;
    }
    case ID_VISIBILITY_OFF:
    {
      mngr.SetCategoryVisible(m_curCategory, false);
      break;
    }

  }
  Invalidate(true);
}

void CategoryForm::OnFormBackRequested(Tizen::Ui::Controls::Form & source)
{
  SceneManager * pSceneManager = SceneManager::GetInstance();
  pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
}

ListItemBase * CategoryForm::CreateItem (int index, float itemWidth)
{
  Bookmark const * pBMark = GetBMManager().GetBookMark(m_curCategory, index);
  String itemText = pBMark->GetName().c_str();
  CustomItem * pItem = new CustomItem();

  pItem->Construct(FloatDimension(itemWidth, lstItmHght), LIST_ANNEX_STYLE_NORMAL);

  int addWdth = 0;
  bool bShowDelete = index == m_itemToDelete;
  if (m_bEditState)
  {
    FloatRectangle imgRect(btwWdth, topHght, imgWdth, imgHght);
    if (bShowDelete)
      pItem->AddElement(imgRect, ID_DELETE, *GetBitmap(IDB_BOOKMARK_DELETE_CUR), null, null);
    else
      pItem->AddElement(imgRect, ID_DELETE, *GetBitmap(IDB_BOOKMARK_DELETE), null, null);
    addWdth = btwWdth + imgWdth;
  }

  FloatRectangle imgRect(addWdth + btwWdth, topHght, imgWdth, imgHght);
  EColor color = fromstringToColor(pBMark->GetType());
  pItem->AddElement(imgRect, ID_COLOR, *GetBitmap(GetColorBM(color)), null, null);
  int beg = addWdth + btwWdth + imgWdth + btwWdth;
  int end = 2 * btwWdth + (bShowDelete ? deleteWidth : distanceWidth);
  pItem->AddElement(FloatRectangle(beg, topHght, itemWidth - beg - end, imgHght), ID_NAME, itemText, mainFontSz, gray, gray, gray);
  if (bShowDelete)
  {
    pItem->AddElement(FloatRectangle(itemWidth - end + btwWdth, topHght, deleteWidth, imgHght), ID_DELETE_TXT, GetString(IDS_DELETE), mainFontSz, red, red, red);
  }
  else
  {
    String dst = GetDistance(pBMark);
    pItem->AddElement(FloatRectangle(itemWidth - btwWdth - distanceWidth, topHght, distanceWidth, imgHght), ID_DISTANCE, dst, mainFontSz, gray, gray, gray);
  }

  return pItem;
}

void CategoryForm::OnListViewItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListItemStatus status)
{
  BookMarkManager & mngr = GetBMManager();
  if (elementId == ID_DELETE)
  {
    if (m_itemToDelete == index)
      m_itemToDelete = -1;
    else
      m_itemToDelete = index;
  }
  if (elementId == ID_DELETE_TXT)
  {
    mngr.DeleteBookMark(m_curCategory, index);
    m_itemToDelete = -1;
  }
  if (elementId == ID_NAME || elementId == ID_DISTANCE)
  {
    mngr.ShowBookMark(m_curCategory, index);
    SceneManager * pSceneManager = SceneManager::GetInstance();
    pSceneManager->ClearSceneHistory();
    pSceneManager->GoForward(ForwardSceneTransition(SCENE_MAP,
        SCENE_TRANSITION_ANIMATION_TYPE_RIGHT, SCENE_HISTORY_OPTION_NO_HISTORY, SCENE_DESTROY_OPTION_DESTROY));
  }
  UpdateState();
}

void CategoryForm::UpdateState()
{
  BookMarkManager & mngr = GetBMManager();
  ListView * pList = static_cast<ListView *>(GetControl(IDC_LISTVIEW, true));
  pList->SetSize(pList->GetWidth(), GetItemCount() * lstItmHght);
  pList->UpdateList();

  CheckButton * pShowOnOffCB = static_cast<CheckButton *>(GetControl(IDC_CHECKBUTTON_SHOW_ON_MAP, true));
  pShowOnOffCB->SetSelected(mngr.IsCategoryVisible(m_curCategory));
  pShowOnOffCB->SetActionId(ID_VISIBILITY_ON, ID_VISIBILITY_OFF);
  pShowOnOffCB->AddActionEventListener(*this);

  EditField * pEditName = static_cast<EditField *>(GetControl(IDC_EDITFIELD_NAME, true));
  pEditName->SetText(mngr.GetCategoryName(m_curCategory));
  GetHeader()->SetTitleText(mngr.GetCategoryName(m_curCategory));
  Header* pHeader = GetHeader();

  ButtonItem buttonItem;
  buttonItem.Construct(BUTTON_ITEM_STYLE_TEXT, ID_EDIT);
  buttonItem.SetText(m_bEditState ? GetString(IDS_DONE): GetString(IDS_EDIT));

  pHeader->SetButton(BUTTON_POSITION_RIGHT, buttonItem);
  Invalidate(true);
}

void CategoryForm::OnTextValueChanged (Tizen::Ui::Control const & source)
{
  EditField * pEditName = static_cast<EditField *>(GetControl(IDC_EDITFIELD_NAME, true));
  GetBMManager().SetCategoryName(m_curCategory, pEditName->GetText());
  UpdateState();
}

bool CategoryForm::DeleteItem (int index, Tizen::Ui::Controls::ListItemBase * pItem, float itemWidth)
{
  delete pItem;
  return true;
}

int CategoryForm::GetItemCount(void)
{
  if (m_curCategory < 0)
    return 0;
  return GetFramework()->GetBmCategory(m_curCategory)->GetBookmarksCount();
}

void CategoryForm::OnSceneActivatedN(const Tizen::Ui::Scenes::SceneId& previousSceneId,
    const Tizen::Ui::Scenes::SceneId& currentSceneId, Tizen::Base::Collection::IList* pArgs)
{
  m_curCategory = -1;
  if (pArgs != null)
  {
    if (pArgs->GetCount() == 1)
    {
      Integer * pCategoryIndex = dynamic_cast<Integer *>(pArgs->GetAt(0));
      m_curCategory = pCategoryIndex->value;
    }
  }

  UpdateState();
}
