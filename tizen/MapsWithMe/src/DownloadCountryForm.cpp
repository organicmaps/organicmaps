#include "DownloadCountryForm.hpp"
#include "SceneRegister.hpp"
#include "MapsWithMeForm.hpp"
#include "AppResourceId.h"
#include "Framework.hpp"
#include "Utils.hpp"
#include "../../../std/bind.hpp"
#include "../../../base/logging.hpp"
#include "../../../platform/settings.hpp"
#include "../../../platform/tizen_utils.hpp"
#include "../../../map/framework.hpp"
#include <FWeb.h>
#include <FAppApp.h>
#include <FApp.h>

using namespace Tizen::Base;
using namespace Tizen::Base::Collection;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;
using namespace Tizen::App;
using namespace Tizen::Web::Controls;
using namespace Tizen::Graphics;
using namespace storage;

DownloadCountryForm::DownloadCountryForm()
: m_downloadedBitmap(0), m_updateBitmap(0)
{
  m_DowloadStatusSlot = Storage().Subscribe(bind(&DownloadCountryForm::OnCountryDownloaded, this, _1),
      bind(&DownloadCountryForm::OnCountryDowloadProgres, this, _1, _2));

  AppResource * pAppResource = Application::GetInstance()->GetAppResource();
  m_downloadedBitmap = pAppResource->GetBitmapN("ic_downloaded_country.png");
  m_updateBitmap = pAppResource->GetBitmapN("ic_update.png");
}

DownloadCountryForm::~DownloadCountryForm(void)
{
  if (m_downloadedBitmap)
    delete (m_downloadedBitmap);
  if (m_updateBitmap)
    delete (m_updateBitmap);
  Storage().Unsubscribe(m_DowloadStatusSlot);
}

bool DownloadCountryForm::Initialize(void)
{
  Construct(IDF_DOWNLOAD_FORM);
  return true;
}

result DownloadCountryForm::OnInitializing(void)
{
  m_group_index = -1;

  SetFormBackEventListener(this);
  return E_SUCCESS;
}

void DownloadCountryForm::OnActionPerformed(const Tizen::Ui::Control & source, int actionId)
{

}

void DownloadCountryForm::OnFormBackRequested(Tizen::Ui::Controls::Form & source)
{
  SceneManager * pSceneManager = SceneManager::GetInstance();
  pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
}

storage::Storage & DownloadCountryForm::Storage() const
{
  return tizen::Framework::GetInstance()->Storage();
}

Tizen::Graphics::Bitmap const * DownloadCountryForm::GetFlag(storage::TIndex const & country)
{
  if (m_Flags.count(country) == 0)
  {
    AppResource * pAppResource = Application::GetInstance()->GetAppResource();
    String sFlagName = "flags/";
    sFlagName += Storage().CountryFlag(country).c_str();
    sFlagName += ".png";
    m_Flags[country] = pAppResource->GetBitmapN(sFlagName);
  }
  return m_Flags[country];
}

Tizen::Ui::Controls::ListItemBase * DownloadCountryForm::CreateItem(int index, float itemWidth)
{
  TIndex country = GetIndex(index);
  FloatDimension itemDimension(itemWidth, 120.0f);
  CustomItem* pItem = new (std::nothrow) CustomItem();
  String sName = Storage().CountryName(country).c_str();

  bool bNeedFlag = GetFlag(country) != 0;
  bool bIsGroup = IsGroup(country);

  ListAnnexStyle style = bIsGroup ? LIST_ANNEX_STYLE_DETAILED : LIST_ANNEX_STYLE_NORMAL;
  pItem->Construct(CoordinateSystem::AlignToDevice(itemDimension), style);

  double dFlagWidth = 60;
  if (bNeedFlag)
  {
    FloatRectangle flagRect(20.0f, 27.0f, dFlagWidth, 60.0f);
    if (GetFlag(country))
      pItem->AddElement(flagRect, ID_FORMAT_FLAG, *GetFlag(country), null, null);
  }

  FloatRectangle nameRect(20.0f, 27.0f, itemWidth - 20 - 60, 60.0f);
  if (bNeedFlag)
  {
    nameRect.x += dFlagWidth + 20;
    nameRect.width -= (dFlagWidth + 200);
  }

  TStatus const status = Storage().CountryStatusEx(country);
  if (status != EDownloadFailed)
    pItem->AddElement(nameRect, ID_FORMAT_STRING, sName, true);
  else
  {
    Tizen::Graphics::Color red(255, 0, 0);
    pItem->AddElement(nameRect, ID_FORMAT_STRING, sName, 45, red, red, red);
  }

  FloatRectangle statusRect(itemWidth - 80.0f, 27.0f, 60, 60.0f);
  if (status == EOnDisk)
  {
    if (m_downloadedBitmap)
      pItem->AddElement(statusRect, ID_FORMAT_STATUS, *m_downloadedBitmap, null, null);
  }
  else if (status == EDownloading || status == EInQueue)
  {
    int pr = 0;

    if (m_lastDownload_value.count(country) > 0)
      pr = 100 * m_lastDownload_value[country].first / m_lastDownload_value[country].second;
    String s;
    s.Append(pr);
    s.Append("%");
    FloatRectangle rect(itemWidth - 100.0f, 27.0f, 90, 60.0f);
    pItem->AddElement(rect, ID_FORMAT_DOWNLOADING_PROGR, s, true);
  }
  else if (status == EOnDiskOutOfDate)
  {
    if (m_updateBitmap)
      pItem->AddElement(statusRect, ID_FORMAT_STATUS, *m_updateBitmap, null, null);
  }

  return pItem;
}

bool DownloadCountryForm::DeleteItem(int index, Tizen::Ui::Controls::ListItemBase * pItem, float itemWidth)
{
  delete pItem;
  pItem = null;
  if (m_Flags.count(GetIndex(index)) != 0)
    delete m_Flags[GetIndex(index)];
  m_Flags.erase(GetIndex(index));
  return true;
}

int DownloadCountryForm::GetItemCount(void)
{
  return Storage().CountriesCount(m_group_index);
}

void DownloadCountryForm::OnSceneActivatedN(const Tizen::Ui::Scenes::SceneId& previousSceneId,
    const Tizen::Ui::Scenes::SceneId& currentSceneId, Tizen::Base::Collection::IList* pArgs)
{
  if (pArgs != null)
  {
    if (pArgs->GetCount() == 3)
    {
      Integer * pLevel = dynamic_cast<Integer *>(pArgs->GetAt(0));
      Integer * pGroup = dynamic_cast<Integer *>(pArgs->GetAt(1));
      Integer * pCountry = dynamic_cast<Integer *>(pArgs->GetAt(2));
      m_group_index.m_group = pGroup->value;
      m_group_index.m_country = pCountry->value;
      m_group_index.m_region = TIndex::INVALID;
      m_valid_values_in_gp_index = pLevel->value;
    }
    ListView * __pList = static_cast<ListView *>(GetControl(IDC_DOWNLOAD_LISTVIEW));
    __pList->SetItemProvider(*this);
    __pList->AddListViewItemEventListener(*this);
    pArgs->RemoveAll(true);
    delete pArgs;
  }

}

void DownloadCountryForm::OnSceneDeactivated(const Tizen::Ui::Scenes::SceneId& currentSceneId,
    const Tizen::Ui::Scenes::SceneId& nextSceneId)
{

}

bool DownloadCountryForm::IsGroup(storage::TIndex const & index) const
{
  return Storage().CountriesCount(index) > 1;
}

TIndex DownloadCountryForm::GetIndex(int const ind) const
{
  TIndex res = m_group_index;
  if (m_valid_values_in_gp_index == 0)
    res.m_group = ind;
  if (m_valid_values_in_gp_index == 1)
    res.m_country = ind;
  else if (m_valid_values_in_gp_index == 2)
    res.m_region = ind;
  return res;
}

wchar_t const * DownloadCountryForm::GetNextScene() const
{
  switch (m_valid_values_in_gp_index)
  {
    case 0:
      return SCENE_DOWNLOAD_COUNTRY;
    case 1:
      return SCENE_DOWNLOAD_REGION;
    default:
      return SCENE_DOWNLOAD_REGION;
  }
}

void DownloadCountryForm::OnListViewItemStateChanged(ListView & listView, int index, int elementId,
    ListItemStatus status)
{
   TIndex country = GetIndex(index);
  if (IsGroup(country))
  {
    ArrayList * pList = new (std::nothrow) ArrayList;
    pList->Construct();
    pList->Add(*(new (std::nothrow) Integer(m_valid_values_in_gp_index + 1)));
    pList->Add(*(new (std::nothrow) Integer(country.m_group)));
    pList->Add(*(new (std::nothrow) Integer(country.m_country)));

    SceneManager * pSceneManager = SceneManager::GetInstance();
    pSceneManager->GoForward(
        ForwardSceneTransition(GetNextScene(), SCENE_TRANSITION_ANIMATION_TYPE_LEFT, SCENE_HISTORY_OPTION_ADD_HISTORY,
            SCENE_DESTROY_OPTION_KEEP), pList);
  }
  else
  {
    TStatus status = Storage().CountryStatusEx(country);
    if (status == ENotDownloaded || status == EDownloadFailed)
    {
      storage::LocalAndRemoteSizeT size = Storage().CountrySizeInBytes(country);
      String msg = GetString(IDS_DOWNLOAD);
      msg.Append(" ");
      msg.Append(int((size.second - size.first) >> 20));
      msg.Append(GetString(IDS_MB));

      if (MessageBoxAsk(Storage().CountryName(country).c_str(), msg))
        Storage().DownloadCountry(country);
    }
    else if (status == EDownloading || status == EInQueue)
    {
      if (MessageBoxAsk(String(Storage().CountryName(country).c_str()), GetString(IDS_CANCEL_DOWNLOAD)))
        if (MessageBoxAsk(String(Storage().CountryName(country).c_str()), GetString(IDS_ARE_YOU_SURE)))
        {
          Storage().DeleteFromDownloader(country);
          m_lastDownload_value.erase(country);
        }
    }
    else if (status == EOnDisk)
    {
      String msg = GetString(IDS_DELETE);
      msg.Append(" ");
      msg.Append(int(Storage().CountrySizeInBytes(country).first >> 20));
      msg.Append(GetString(IDS_MB));
      if (MessageBoxAsk(String(Storage().CountryName(country).c_str()), msg))
        if (MessageBoxAsk(String(Storage().CountryName(country).c_str()), GetString(IDS_ARE_YOU_SURE)))
          tizen::Framework::GetInstance()->DeleteCountry(country);
    }
    UpdateList();
  }
}

void DownloadCountryForm::UpdateList()
{
  ListView * __pList = static_cast<ListView *>(GetControl(IDC_DOWNLOAD_LISTVIEW));
  __pList->UpdateList();
}

void DownloadCountryForm::OnCountryDownloaded(TIndex const & country)
{
  if (Storage().CountryStatusEx(country) == EDownloadFailed)
  {
    bool static bOddEnterHack = true; // message about download fail comes twice. Hack for showing only one of them
    bOddEnterHack = !bOddEnterHack;
    if (bOddEnterHack)
    {
      String sName = Storage().CountryName(country).c_str();
      MessageBoxOk(sName, FormatString1(IDS_DOWNLOAD_COUNTRY_FAILED, sName));
    }
  }
  UpdateList();
}

void DownloadCountryForm::OnCountryDowloadProgres(TIndex const & index, pair<int64_t, int64_t> const & p)
{
  m_lastDownload_value[index] = p;
  UpdateList();
}

void DownloadCountryForm::OnListViewItemSwept(ListView & listView, int index, SweepDirection direction)
{
}

void DownloadCountryForm::OnListViewContextItemStateChanged(ListView & listView, int index, int elementId,
    ListContextItemStatus state)
{
}

void DownloadCountryForm::OnListViewItemLongPressed(ListView & listView, int index, int elementId,
    bool & invokeListViewItemCallback)
{
}
