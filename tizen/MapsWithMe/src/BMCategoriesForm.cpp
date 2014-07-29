#include "BMCategoriesForm.hpp"
#include "SceneRegister.hpp"
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
using namespace Tizen::Base::Collection;
using namespace consts;
using namespace bookmark;

BMCategoriesForm::BMCategoriesForm()
{
}

BMCategoriesForm::~BMCategoriesForm(void)
{
}

bool BMCategoriesForm::Initialize(void)
{
  Construct(IDF_BMCATEGORIES_FORM);
  return true;
}

result BMCategoriesForm::OnInitializing(void)
{
  m_bEditState = false;
  m_categoryToDelete = -1;

  ListView * pList = static_cast<ListView *>(GetControl(IDC_LISTVIEW, true));
  pList->SetItemProvider(*this);
  pList->AddListViewItemEventListener(*this);

  GetHeader()->AddActionEventListener(*this);

  SetFormBackEventListener(this);
  UpdateState();
  return E_SUCCESS;
}

void BMCategoriesForm::OnActionPerformed(Tizen::Ui::Control const & source, int actionId)
{
  switch(actionId)
  {
    case ID_EDIT:
    {
      m_bEditState = !m_bEditState;
      m_categoryToDelete = -1;
      UpdateState();
      break;
    }
  }
  Invalidate(true);
}

void BMCategoriesForm::OnFormBackRequested(Tizen::Ui::Controls::Form & source)
{
  SceneManager * pSceneManager = SceneManager::GetInstance();
  pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
}

ListItemBase * BMCategoriesForm::CreateItem (int index, float itemWidth)
{
  CustomItem * pItem = new CustomItem();

  BookMarkManager & mngr = GetBMManager();
  pItem->Construct(FloatDimension(itemWidth, lstItmHght), LIST_ANNEX_STYLE_NORMAL);

  int addWdth = 0;
  bool bShowDelete = index == m_categoryToDelete;
  if (m_bEditState)
  {
    FloatRectangle imgRect(btwWdth, topHght, imgWdth, imgHght);
    pItem->AddElement(imgRect, ID_DELETE, *GetBitmap(bShowDelete ? IDB_BOOKMARK_DELETE_CUR : IDB_BOOKMARK_DELETE), null, null);
    addWdth = btwWdth + imgWdth;
  }

  FloatRectangle imgRect(addWdth + btwWdth, topHght, imgWdth, imgHght);
  pItem->AddElement(imgRect, ID_EYE, *GetBitmap(mngr.IsCategoryVisible(index) ? IDB_EYE : IDB_EMPTY), null, null);
  int beg = addWdth + btwWdth + imgWdth + btwWdth;
  int end = 2 * btwWdth + (bShowDelete ? deleteWidth : imgWdth);
  String itemText = mngr.GetCategoryName(index);
  pItem->AddElement(FloatRectangle(beg, topHght, itemWidth - beg - end, imgHght), ID_NAME, itemText, mainFontSz, gray, gray, gray);
  if (bShowDelete)
  {
    pItem->AddElement(FloatRectangle(itemWidth - end + btwWdth, topHght, deleteWidth, imgHght), ID_DELETE_TXT, "Delete", mainFontSz, red, red, red);
  }
  else
  {
    int count = mngr.GetCategorySize(index);
    String sz;
    sz.Append(count);
    pItem->AddElement(FloatRectangle(itemWidth - btwWdth - imgWdth, topHght, imgWdth, imgHght), ID_SIZE, sz, mainFontSz, gray, gray, gray);
  }

  return pItem;
}

void BMCategoriesForm::OnListViewItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListItemStatus status)
{
  BookMarkManager & mngr = GetBMManager();
  if (elementId == ID_EYE)
  {
    bool bVisible = mngr.IsCategoryVisible(index);
    mngr.SetCategoryVisible(index, !bVisible);
  }
  if (elementId == ID_DELETE)
  {
    if (m_categoryToDelete == index)
      m_categoryToDelete = -1;
    else
      m_categoryToDelete = index;
  }
  if (elementId == ID_DELETE_TXT)
  {
    mngr.DeleteCategory(index);
    m_categoryToDelete = -1;
  }

  if (elementId == ID_NAME || elementId == ID_SIZE)
  {
    ArrayList * pList = new ArrayList;
    pList->Construct();
    pList->Add(new Integer(index));
    SceneManager * pSceneManager = SceneManager::GetInstance();
    pSceneManager->GoForward(ForwardSceneTransition(SCENE_CATEGORY,
        SCENE_TRANSITION_ANIMATION_TYPE_LEFT, SCENE_HISTORY_OPTION_ADD_HISTORY, SCENE_DESTROY_OPTION_DESTROY), pList);
    return;
  }
  UpdateState();
}

void BMCategoriesForm::UpdateState()
{
  ListView * pList = static_cast<ListView *>(GetControl(IDC_LISTVIEW, true));
  pList->UpdateList();

  Header* pHeader = GetHeader();

  ButtonItem buttonItem;
  buttonItem.Construct(BUTTON_ITEM_STYLE_TEXT, ID_EDIT);
  buttonItem.SetText(m_bEditState ? GetString(IDS_DONE): GetString(IDS_EDIT));

  pHeader->SetButton(BUTTON_POSITION_RIGHT, buttonItem);
  Invalidate(true);
}

bool BMCategoriesForm::DeleteItem (int index, Tizen::Ui::Controls::ListItemBase * pItem, float itemWidth)
{
  delete pItem;
  return true;
}

int BMCategoriesForm::GetItemCount(void)
{
  return GetBMManager().GetCategoriesCount();
}
