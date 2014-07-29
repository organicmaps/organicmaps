#include "SelectBMCategoryForm.hpp"
#include "SceneRegister.hpp"
#include "MapsWithMeForm.hpp"
#include "AppResourceId.h"
#include "Utils.hpp"
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

SelectBMCategoryForm::SelectBMCategoryForm()
{
}

SelectBMCategoryForm::~SelectBMCategoryForm(void)
{
}

bool SelectBMCategoryForm::Initialize(void)
{
  Construct(IDF_SELECT_BM_CATEGORY_FORM);
  return true;
}

result SelectBMCategoryForm::OnInitializing(void)
{
  EditField * pEditField = static_cast<EditField*>(GetControl(IDC_EDITFIELD, true));
  pEditField->AddTextEventListener(*this);

  ListView * pList = static_cast<ListView *>(GetControl(IDC_LISTVIEW, true));
  pList->SetItemProvider(*this);
  pList->AddListViewItemEventListener(*this);
  SetFormBackEventListener(this);

  return E_SUCCESS;
}

void SelectBMCategoryForm::OnFormBackRequested(Tizen::Ui::Controls::Form & source)
{
  SceneManager * pSceneManager = SceneManager::GetInstance();
  pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
}

ListItemBase * SelectBMCategoryForm::CreateItem(int index, float itemWidth)
{
  String itemText = GetBMManager().GetCategoryName(index);
  CustomItem * pItem = new CustomItem();

  pItem->Construct(FloatDimension(itemWidth, lstItmHght), LIST_ANNEX_STYLE_NORMAL);
  FloatRectangle imgRect(btwWdth, topHght, imgWdth, imgHght);
  pItem->AddElement(FloatRectangle(btwWdth, topHght, itemWidth - 2 * btwWdth - imgWdth, imgHght), 1, itemText, mainFontSz, black, black, black);
  if(index == GetBMManager().GetCurrentCategory())
    pItem->AddElement(FloatRectangle(itemWidth - btwWdth - imgWdth, topHght, imgWdth, imgHght), 0, *GetBitmap(IDB_V));

  return pItem;
}

void SelectBMCategoryForm::OnListViewItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListItemStatus status)
{
  GetBMManager().SetNewCurBookMarkCategory(index);
  SceneManager * pSceneManager = SceneManager::GetInstance();
  pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
}

bool SelectBMCategoryForm::DeleteItem(int index, Tizen::Ui::Controls::ListItemBase * pItem, float itemWidth)
{
  delete pItem;
  return true;
}

int SelectBMCategoryForm::GetItemCount(void)
{
  return GetBMManager().GetCategoriesCount();
}

void SelectBMCategoryForm::OnTextValueChangeCanceled(Tizen::Ui::Control const & source)
{
  EditField * pEditField = static_cast<EditField *>(GetControl(IDC_EDITFIELD, true));
  pEditField->SetText("");
  Invalidate(true);
}

void SelectBMCategoryForm::OnTextValueChanged(Tizen::Ui::Control const & source)
{
  EditField * pEditField = static_cast<EditField *>(GetControl(IDC_EDITFIELD, true));
  int iNewCat = GetBMManager().AddCategory(pEditField->GetText());
  if (iNewCat >= 0)
    GetBMManager().SetNewCurBookMarkCategory(iNewCat);
  pEditField->SetText("");
  ListView * pList = static_cast<ListView *>(GetControl(IDC_LISTVIEW, true));
  pList->UpdateList();
  Invalidate(true);
}
