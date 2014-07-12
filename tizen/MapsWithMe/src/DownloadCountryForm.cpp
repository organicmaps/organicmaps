#include "DownloadCountryForm.hpp"
#include "SceneRegister.hpp"
#include "MapsWithMeForm.hpp"
#include "AppResourceId.h"
#include "Framework.hpp"
#include "Utils.hpp"
#include "FormFactory.hpp"

#include "../../../map/framework.hpp"
#include "../../../platform/settings.hpp"
#include "../../../platform/tizen_utils.hpp"
#include "../../../base/logging.hpp"
#include "../../../std/bind.hpp"

#include <FWeb.h>
#include <FAppApp.h>
#include <FApp.h>
#include <FNetNetConnectionManager.h>

using namespace Tizen::Base;
using namespace Tizen::Base::Collection;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;
using namespace Tizen::App;
using namespace Tizen::Net;
using namespace Tizen::Web::Controls;
using namespace Tizen::Graphics;
using namespace storage;

DownloadCountryForm::DownloadCountryForm()
    : m_downloadedBitmap(0), m_updateBitmap(0)
{
  m_dowloadStatusSlot = Storage().Subscribe(bind(&DownloadCountryForm::OnCountryDownloaded, this, _1),
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
  Storage().Unsubscribe(m_dowloadStatusSlot);
}

bool DownloadCountryForm::Initialize(void)
{
  Construct(IDF_DOWNLOAD_FORM);
  return true;
}

result DownloadCountryForm::OnInitializing(void)
{
  m_groupIndex = -1;

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
  if (m_flags.count(country) == 0)
  {
    AppResource * pAppResource = Application::GetInstance()->GetAppResource();
    String sFlagName = "flags/";
    sFlagName += Storage().CountryFlag(country).c_str();
    sFlagName += ".png";
    m_flags[country] = pAppResource->GetBitmapN(sFlagName);
  }
  return m_flags[country];
}

Tizen::Ui::Controls::ListItemBase * DownloadCountryForm::CreateItem(int index, float itemWidth)
{
  TIndex country = GetIndex(index);
  FloatDimension itemDimension(itemWidth, 120.0f);
  CustomItem * pItem = new CustomItem();
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

    if (m_lastDownloadValue.count(country) > 0)
      pr = 100 * m_lastDownloadValue[country].first / m_lastDownloadValue[country].second;
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
  TIndex const ind = GetIndex(index);
  if (m_flags.count(ind) != 0)
    delete m_flags[ind];
  m_flags.erase(ind);
  return true;
}

int DownloadCountryForm::GetItemCount(void)
{
  return Storage().CountriesCount(m_groupIndex);
}

void DownloadCountryForm::OnSceneActivatedN(const Tizen::Ui::Scenes::SceneId& previousSceneId,
    const Tizen::Ui::Scenes::SceneId& currentSceneId, Tizen::Base::Collection::IList* pArgs)
{
  m_fromId = SceneManager::GetInstance()->GetCurrentScene()->GetFormId();
  if (pArgs != null)
  {
    if (pArgs->GetCount() == 2)
    {
      Integer * pGroup = dynamic_cast<Integer *>(pArgs->GetAt(0));
      Integer * pCountry = dynamic_cast<Integer *>(pArgs->GetAt(1));
      m_groupIndex.m_group = pGroup->value;
      m_groupIndex.m_country = pCountry->value;
      m_groupIndex.m_region = TIndex::INVALID;
    }
    ListView *pList = static_cast<ListView *>(GetControl(IDC_DOWNLOAD_LISTVIEW));
    pList->SetItemProvider(*this);
    pList->AddListViewItemEventListener(*this);
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
  TIndex res = m_groupIndex;
  if (m_fromId == FORM_DOWNLOAD_GROUP)
    res.m_group = ind;
  if (m_fromId == FORM_DOWNLOAD_COUNTRY)
    res.m_country = ind;
  else if (m_fromId == FORM_DOWNLOAD_REGION)
    res.m_region = ind;
  return res;
}

wchar_t const * DownloadCountryForm::GetNextScene() const
{
  if (m_fromId == FORM_DOWNLOAD_GROUP)
    return SCENE_DOWNLOAD_COUNTRY;
  return SCENE_DOWNLOAD_REGION;
}

void DownloadCountryForm::OnListViewItemStateChanged(ListView & listView, int index, int elementId,
    ListItemStatus status)
{
  TIndex country = GetIndex(index);
  if (IsGroup(country))
  {
    ArrayList * pList = new ArrayList;
    pList->Construct();
    pList->Add(new Integer(country.m_group));
    pList->Add(new Integer(country.m_country));

    SceneManager * pSceneManager = SceneManager::GetInstance();
    pSceneManager->GoForward(
        ForwardSceneTransition(GetNextScene(), SCENE_TRANSITION_ANIMATION_TYPE_LEFT, SCENE_HISTORY_OPTION_ADD_HISTORY,
            SCENE_DESTROY_OPTION_KEEP), pList);
  }
  else
  {
    String name = Storage().CountryName(country).c_str();
    TStatus status = Storage().CountryStatusEx(country);
    if (status == ENotDownloaded || status == EDownloadFailed)
    {
      storage::LocalAndRemoteSizeT size = Storage().CountrySizeInBytes(country);
      int const sz_in_MB = int((size.second - size.first) >> 20);
      String msg = GetString(IDS_DOWNLOAD);
      msg.Append(" ");
      msg.Append(sz_in_MB);
      msg.Append(GetString(IDS_MB));

      NetConnectionManager connectionManager;
      connectionManager.Construct();
      ManagedNetConnection * pManagedNetConnection = connectionManager.GetManagedNetConnectionN();
      NetConnectionInfo const * pInfo = pManagedNetConnection->GetNetConnectionInfo();

      bool bDownload = true;
      if (pInfo == 0 || pInfo->GetBearerType() == NET_BEARER_NONE)
      {
        bDownload = false;
        MessageBoxOk(GetString(IDS_NO_INTERNET_CONNECTION_DETECTED), GetString(IDS_USE_WIFI_RECOMMENDATION_TEXT));
      }
      if (bDownload && pInfo->GetBearerType() != NET_BEARER_WIFI && (sz_in_MB > 10))
        bDownload = MessageBoxAsk(name, FormatString1(IDS_NO_WIFI_ASK_CELLULAR_DOWNLOAD, name));

      if (bDownload && MessageBoxAsk(name, msg))
        Storage().DownloadCountry(country);
    }
    else if (status == EDownloading || status == EInQueue)
    {
      if (MessageBoxAsk(name, GetString(IDS_CANCEL_DOWNLOAD)))
        if (MessageBoxAsk(name, GetString(IDS_ARE_YOU_SURE)))
        {
          Storage().DeleteFromDownloader(country);
          m_lastDownloadValue.erase(country);
        }
    }
    else if (status == EOnDisk)
    {
      String msg = GetString(IDS_DELETE);
      msg.Append(" ");
      msg.Append(int(Storage().CountrySizeInBytes(country).first >> 20));
      msg.Append(GetString(IDS_MB));
      if (MessageBoxAsk(name, msg))
        if (MessageBoxAsk(name, GetString(IDS_ARE_YOU_SURE)))
          tizen::Framework::GetInstance()->DeleteCountry(country);
    }
    UpdateList();
  }
}

void DownloadCountryForm::UpdateList()
{
  ListView * pList = static_cast<ListView *>(GetControl(IDC_DOWNLOAD_LISTVIEW));
  pList->UpdateList();
}

void DownloadCountryForm::OnCountryDownloaded(TIndex const & country)
{
  if (m_fromId != SceneManager::GetInstance()->GetCurrentScene()->GetFormId())
    return;
  if (Storage().CountryStatusEx(country) == EDownloadFailed)
  {
    String sName = Storage().CountryName(country).c_str();
    MessageBoxOk(sName, FormatString1(IDS_DOWNLOAD_COUNTRY_FAILED, sName));
  }
  UpdateList();
}

void DownloadCountryForm::OnCountryDowloadProgres(TIndex const & index, pair<int64_t, int64_t> const & p)
{
  if (m_fromId != SceneManager::GetInstance()->GetCurrentScene()->GetFormId())
    return;
  m_lastDownloadValue[index] = p;
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
