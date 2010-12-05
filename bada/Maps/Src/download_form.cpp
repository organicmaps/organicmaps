#include "download_form.h"
#include "maps_gl.h"

#include "../../../networking/map_storage.hpp"

#include <FApp.h>

using namespace Osp::Ui;
using namespace Osp::Ui::Controls;
using namespace Osp::Graphics;
using namespace Osp::Base;

DownloadForm::DownloadForm(MapsGl & mapsGl)
    : m_mapsGl(mapsGl), //__pLabelLog(null),
      __pItemFormat(null), __pGroupedList(null)
{
}

DownloadForm::~DownloadForm()
{
  // use OnTerminating() instead
}

bool DownloadForm::Initialize(void)
{
  Construct(FORM_STYLE_NORMAL | FORM_STYLE_TITLE | FORM_STYLE_INDICATOR
      | FORM_STYLE_SOFTKEY_1);
  SetTitleText(L"Countries");

  SetSoftkeyText(SOFTKEY_1, L"Back");
  SetSoftkeyActionId(SOFTKEY_1, ID_BACK_SOFTKEY);
  AddSoftkeyActionListener(SOFTKEY_1, *this);

  return true;
}

result DownloadForm::OnInitializing(void)
{
  result r = E_SUCCESS;

  // Get Bitmap
//  Bitmap *pHome = GetBitmapN(L"home.png");
//  Bitmap *pMsg = GetBitmapN(L"message.png");
//  Bitmap *pAlarm = GetBitmapN(L"alarm.png");
//
//  Bitmap *pHome_focused = GetBitmapN(L"home_focused.png");
//  Bitmap *pMsg_focused = GetBitmapN(L"message_focused.png");
//  Bitmap *pAlarm_focused = GetBitmapN(L"alarm_focused.png");

  // Create CustomItem and format
  __pItemFormat = new CustomListItemFormat();
  __pItemFormat->Construct();

//  __pItemFormat->AddElement(ID_FORMAT_STRING, Rectangle(110, 30, 250, 50),38, Color::COLOR_WHITE, Color::COLOR_BLUE);
//	__pItemFormat->AddElement(ID_FORMAT_BITMAP, Rectangle(22, 22, 56, 56));
  __pItemFormat->AddElement(ID_FORMAT_STRING, Rectangle(25, 35, 480 - 25, 50));
  __pItemFormat->SetElementEventEnabled(ID_FORMAT_STRING, true);
//	__pItemFormat->SetElementEventEnabled(ID_FORMAT_BITMAP, true);

	// Create GroupedList control
	__pGroupedList = new GroupedList();
//	__pGroupedList->Construct(Rectangle(0, 200, 480, GetClientAreaBounds().height - 200), CUSTOM_LIST_STYLE_NORMAL, true, true);
  __pGroupedList->Construct(Rectangle(0, 0, 480, GetClientAreaBounds().height), CUSTOM_LIST_STYLE_NORMAL, true, true);
	__pGroupedList->AddGroupedItemEventListener(*this);

	ReFillList(*__pGroupedList);
//	__pGroupedList->SetFastScrollMainIndex("AB");
//	__pGroupedList->SetFastScrollSubIndex("123,12", SCROLL_INDEX_DIGIT_NUM_1);
//	__pGroupedList->AddFastScrollEventListener(*this);
	AddControl(*__pGroupedList);

	// Add Label Control
//	__pLabelLog = new Label();
//	__pLabelLog->Construct(Rectangle(30, 10, 450, 60), L"Log");
//	__pLabelLog->SetTextColor(Color::COLOR_WHITE);
//	__pLabelLog->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
//	AddControl(*__pLabelLog);

	// Deallocate bitmaps
//	delete pHome;
//	delete pMsg;
//	delete pAlarm;
//	delete pHome_focused;
//	delete pMsg_focused;
//	delete pAlarm_focused;

	return r;
}

result DownloadForm::ReFillList(GroupedList & list)
{
  list.RemoveAllItems();
  list.RemoveAllGroups();
  networking::MapsInformationStorage & storage = networking::MapsInformationStorage::Instance();
  networking::StringContainer groups;
  size_t groupsCount = storage.GetGroups(groups);
  for (size_t i = 0; i < groupsCount; ++i)
  {
    list.AddGroup(groups[i].c_str(), null);

    networking::StringContainer countries;
    size_t countriesCount = storage.GetMapsForGroup(groups[i], countries);
    for (size_t j = 0; j < countriesCount; ++j)
    {
      CustomListItem * pItem = new CustomListItem();
      pItem->Construct(100);
      pItem->SetItemFormat(*__pItemFormat);
      pItem->SetElement(ID_FORMAT_STRING, countries[j].c_str());
      list.AddItem(i, *pItem);
    }
  }
  return E_SUCCESS;
}

result DownloadForm::OnTerminating(void)
{
  delete __pItemFormat;
  return E_SUCCESS;
}

void CloseDownloadForm()
{
  Frame * pFrame = Osp::App::Application::GetInstance()->GetAppFrame()->GetFrame();
  Control * pMapsForm = pFrame->GetControl("MapsForm");
  if (pMapsForm != null)
    pMapsForm->SendUserEvent(DownloadForm::REQUEST_MAINFORM, null);
}

void DownloadForm::OnActionPerformed(const Control& source, int actionId)
{
  switch (actionId)
  {
    case ID_BACK_SOFTKEY:
    {
      CloseDownloadForm();
    }
    break;
  }
}

result DownloadForm::ShowCountryAndClose(int groupIndex, int itemIndex)
{
  networking::MapsInformationStorage & storage = networking::MapsInformationStorage::Instance();
  networking::StringContainer groups;
  if (groupIndex < static_cast<int>(storage.GetGroups(groups)))
  {
    networking::StringContainer countries;
    if (itemIndex < static_cast<int>(storage.GetMapsForGroup(groups[groupIndex], countries)))
    {
      m_mapsGl.Framework()->SetCountryRect(countries[itemIndex].c_str());
      CloseDownloadForm();
    }
  }
  return E_SUCCESS;
}

void DownloadForm::OnItemStateChanged(const Osp::Ui::Control &source,
    int groupIndex, int itemIndex, int itemId, Osp::Ui::ItemStatus status)
{
  // user selected country in the list
  if (status == ITEM_SELECTED)
  {
    ShowCountryAndClose(groupIndex, itemIndex);
  }
//  String itemText("");
//  switch (itemId)
//  {
//  case ID_CUSTOMLIST_ITEM1:
//    itemText.Format(20, L"Item %d: HOME", itemIndex+1);
//    break;
//    case ID_CUSTOMLIST_ITEM2:
//    itemText.Format(20,L"Item %d: MESSAGE", itemIndex+1);
//    break;
//    case ID_CUSTOMLIST_ITEM3:
//    itemText.Format(20,L"Item %d: ALARM", itemIndex+1);
//    break;
//    case ID_CUSTOMLIST_ITEM4:
//    itemText.Format(20,L"Item %d: HOME", itemIndex+1);
//    break;
//    case ID_CUSTOMLIST_ITEM5:
//    itemText.Format(20,L"Item %d: MESSAGE", itemIndex+1);
//    break;
//    default:
//    break;
//  }
//  __pLabelLog->SetText(itemText);
//  __pLabelLog->Draw();
//  __pLabelLog->Show();
}

void DownloadForm::OnItemStateChanged(const Osp::Ui::Control &source,
    int groupIndex, int itemIndex, int itemId, int elementId,
    Osp::Ui::ItemStatus status)
{
  if (status == ITEM_SELECTED)
  {
    ShowCountryAndClose(groupIndex, itemIndex);
  }
//  String itemText("");
//  switch (itemId)
//  {
//  case ID_CUSTOMLIST_ITEM1:
//    switch (elementId)
//    {
//    case ID_FORMAT_STRING:
//      itemText.Format(50, L"Item %d: Text Selected", itemIndex+1);
//      break;
//
//      case ID_FORMAT_BITMAP:
//      itemText.Format(50, L"Item %d: Bitmap Selected", itemIndex+1);
//      break;
//
//      default:
//      break;
//    }
//    break;
//    case ID_CUSTOMLIST_ITEM2:
//    switch (elementId)
//    {
//      case ID_FORMAT_STRING:
//      itemText.Format(50, L"Item %d: Text Selected", itemIndex+1);
//      break;
//
//      case ID_FORMAT_BITMAP:
//      itemText.Format(50, L"Item %d: Bitmap Selected", itemIndex+1);
//      break;
//
//      default:
//      break;
//    }
//    break;
//    case ID_CUSTOMLIST_ITEM3:
//    switch (elementId)
//    {
//      case ID_FORMAT_STRING:
//      itemText.Format(50, L"Item %d: Text Selected", itemIndex+1);
//      break;
//
//      case ID_FORMAT_BITMAP:
//      itemText.Format(50, L"Item %d: Bitmap Selected", itemIndex+1);
//      break;
//
//      default:
//      break;
//    }
//    break;
//    case ID_CUSTOMLIST_ITEM4:
//    switch (elementId)
//    {
//      case ID_FORMAT_STRING:
//      itemText.Format(50, L"Item %d: Text Selected", itemIndex+1);
//      break;
//
//      case ID_FORMAT_BITMAP:
//      itemText.Format(50, L"Item %d: Bitmap Selected", itemIndex+1);
//      break;
//
//      default:
//      break;
//    }
//    break;
//    case ID_CUSTOMLIST_ITEM5:
//    switch (elementId)
//    {
//      case ID_FORMAT_STRING:
//      itemText.Format(50, L"Item %d: Text Selected", itemIndex+1);
//      break;
//
//      case ID_FORMAT_BITMAP:
//      itemText.Format(50, L"Item %d: Bitmap Selected", itemIndex+1);
//      break;
//
//      default:
//      break;
//    }
//    break;
//    default:
//    break;
//  }
//  __pLabelLog->SetText(itemText);
//
//  __pLabelLog->Draw();
//  __pLabelLog->Show();
}

//void DownloadForm::OnMainIndexChanged(const Osp::Ui::Control &source,
//    Osp::Base::Character &mainIndex)
//{
//  if (!mainIndex.CompareTo(Osp::Base::Character('A')))
//    __pGroupedList->ScrollToTop(0);
//  else if (!mainIndex.CompareTo(Osp::Base::Character('B')))
//    __pGroupedList->ScrollToTop(1);
//
//  __pGroupedList->Draw();
//  __pGroupedList->Show();
//}
//
//void DownloadForm::OnSubIndexChanged(const Osp::Ui::Control &source,
//    Osp::Base::Character &mainIndex, Osp::Base::Character &subIndex)
//{
//  if (!mainIndex.CompareTo(Osp::Base::Character('A')))
//  {
//    if (!subIndex.CompareTo(Osp::Base::Character('1')))
//      __pGroupedList->ScrollToTop(0, 0);
//    else if (!subIndex.CompareTo(Osp::Base::Character('2')))
//      __pGroupedList->ScrollToTop(0, 1);
//    else if (!subIndex.CompareTo(Osp::Base::Character('3')))
//      __pGroupedList->ScrollToTop(0, 2);
//  }
//  else if (!mainIndex.CompareTo(Osp::Base::Character('B')))
//  {
//    if (!subIndex.CompareTo(Osp::Base::Character('1')))
//      __pGroupedList->ScrollToTop(1, 0);
//    else if (!subIndex.CompareTo(Osp::Base::Character('2')))
//      __pGroupedList->ScrollToTop(1, 1);
//  }
//  __pGroupedList->Draw();
//  __pGroupedList->Show();
//}
//
//void DownloadForm::OnMainIndexSelected(const Osp::Ui::Control &source,
//    Osp::Base::Character &mainIndex)
//{
//}
//
//void DownloadForm::OnSubIndexSelected(const Osp::Ui::Control &source,
//    Osp::Base::Character &mainIndex, Osp::Base::Character &subIndex)
//{
//}
