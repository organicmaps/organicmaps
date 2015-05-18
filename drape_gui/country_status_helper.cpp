#include "country_status_helper.hpp"
#include "drape_gui.hpp"

#include "storage/index.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"
#include "base/string_format.hpp"

namespace gui
{

namespace
{

CountryStatusHelper::Control MakeLabel(string const & text)
{
  return { text, CountryStatusHelper::CONTROL_TYPE_LABEL, CountryStatusHelper::BUTTON_TYPE_NOT_BUTTON };
}

CountryStatusHelper::Control MakeButton(string const & text, CountryStatusHelper::EButtonType type)
{
  return { text, CountryStatusHelper::CONTROL_TYPE_BUTTON, type };
}

CountryStatusHelper::Control MakeProgress()
{
  return { "", CountryStatusHelper::CONTROL_TYPE_PROGRESS, CountryStatusHelper::BUTTON_TYPE_NOT_BUTTON };
}

string GetLocalizedString(string const & id)
{
  return DrapeGui::Instance().GetLocalizedString(id);
}

void FormatMapSize(uint64_t sizeInBytes, string & units, size_t & sizeToDownload)
{
  int const mbInBytes = 1024 * 1024;
  int const kbInBytes = 1024;
  if (sizeInBytes > mbInBytes)
  {
    sizeToDownload = (sizeInBytes + mbInBytes - 1) / mbInBytes;
    units = "MB";
  }
  else if (sizeInBytes > kbInBytes)
  {
    sizeToDownload = (sizeInBytes + kbInBytes -1) / kbInBytes;
    units = "KB";
  }
  else
  {
    sizeToDownload = sizeInBytes;
    units = "B";
  }
}

char const * DownloadMapButtonID = "country_status_download";
char const * DownloadMapRoutingButtonID = "country_status_download_routing";
char const * TryAgainButtonID = "try_again";
char const * DownloadingLabelID = "country_status_downloading";
char const * DownloadingFailedID = "country_status_download_failed";
char const * InQueueID = "country_status_added_to_queue";

} // namespace

////////////////////////////////////////////////////////////

CountryStatusHelper::CountryStatusHelper()
  : m_state(COUNTRY_STATE_LOADED)
{
}

void CountryStatusHelper::SetStorageAccessor(ref_ptr<StorageAccessor> accessor)
{
  m_accessor = accessor;
}

void CountryStatusHelper::SetCountryIndex(storage::TIndex const & index)
{
  ASSERT(m_accessor != nullptr, ());
  if (m_accessor->GetCountryIndex() == index)
    return;

  CountryStatusHelper::ECountryState state = CountryStatusHelper::COUNTRY_STATE_LOADED;
  m_accessor->SetCountryIndex(index);
  switch(m_accessor->GetCountryStatus())
  {
  case storage::TStatus::ENotDownloaded:
    state = CountryStatusHelper::COUNTRY_STATE_EMPTY;
    break;
  case storage::TStatus::EDownloading:
    state = CountryStatusHelper::COUNTRY_STATE_LOADING;
    break;
  case storage::TStatus::EInQueue:
    state = CountryStatusHelper::COUNTRY_STATE_IN_QUEUE;
    break;
  case storage::TStatus::EDownloadFailed:
  case storage::TStatus::EOutOfMemFailed:
    state = CountryStatusHelper::COUNTRY_STATE_FAILED;
    break;
  default:
    break;
  }

  SetState(state);
}

storage::TIndex CountryStatusHelper::GetCountryIndex() const
{
  ASSERT(m_accessor != nullptr, ());
  return m_accessor->GetCountryIndex();
}

void CountryStatusHelper::SetState(ECountryState state)
{
  m_state = state;
  FillControlsForState();
  DrapeGui::Instance().EmitRecacheSignal(Skin::CountryStatus);
}

CountryStatusHelper::ECountryState CountryStatusHelper::GetState() const
{
  return m_state;
}

bool CountryStatusHelper::IsVisibleForState(ECountryState state) const
{
  return m_state != COUNTRY_STATE_LOADED && m_state == state;
}

size_t CountryStatusHelper::GetComponentCount() const { return m_controls.size(); }

CountryStatusHelper::Control const & CountryStatusHelper::GetControl(size_t index) const
{
  return m_controls[index];
}

float CountryStatusHelper::GetControlMargin()
{
  return 5.0f * DrapeGui::Instance().GetScaleFactor();
}

void CountryStatusHelper::GetProgressInfo(string & alphabet, size_t & maxLength)
{
  alphabet = " 0123456789%";
  maxLength = 5;
}

string CountryStatusHelper::GetProgressValue() const
{
  return strings::to_string(m_accessor->GetDownloadProgress()) + "%";
}

void CountryStatusHelper::FillControlsForState()
{
  m_controls.clear();
  ECountryState state = m_state;
  switch (state)
  {
  case COUNTRY_STATE_EMPTY:
    FillControlsForEmpty();
    break;
  case COUNTRY_STATE_LOADING:
    FillControlsForLoading();
    break;
  case COUNTRY_STATE_IN_QUEUE:
    FillControlsForInQueue();
    break;
  case COUNTRY_STATE_FAILED:
    FillControlsForFailed();
    break;
  default:
    break;
  }
}

void CountryStatusHelper::FillControlsForEmpty()
{
  ASSERT(m_controls.empty(), ());
  m_controls.push_back(MakeLabel(m_accessor->GetCurrentCountryName()));
  m_controls.push_back(MakeButton(FormatDownloadMap(), BUTTON_TYPE_MAP));
  m_controls.push_back(MakeButton(FormatDownloadMapRouting(), BUTTON_TYPE_MAP_ROUTING));
}

void CountryStatusHelper::FillControlsForLoading()
{
  ASSERT(m_controls.empty(), ());
  string text = GetLocalizedString(DownloadingLabelID);
  size_t firstPos = text.find('^');
  ASSERT(firstPos != string::npos, ());
  size_t secondPos = text.find('^', firstPos + 1);
  ASSERT(secondPos != string::npos, ());

  if (firstPos != 0)
  {
    string firstLabel = text.substr(0, firstPos);
    strings::Trim(firstLabel, "\n ");
    m_controls.push_back(MakeLabel(firstLabel));
  }

  m_controls.push_back(MakeLabel(m_accessor->GetCurrentCountryName()));
  m_controls.push_back(MakeProgress());

  if (secondPos + 1 < text.size())
  {
    string secondLabel = text.substr(secondPos + 1);
    strings::Trim(secondLabel , "\n ");
    m_controls.push_back(MakeLabel(secondLabel));
  }
}

void CountryStatusHelper::FillControlsForInQueue()
{
  ASSERT(m_controls.empty(), ());
  m_controls.push_back(MakeLabel(FormatInQueueMap()));
}

void CountryStatusHelper::FillControlsForFailed()
{
  ASSERT(m_controls.empty(), ());
  m_controls.push_back(MakeLabel(FormatFailed()));
  m_controls.push_back(MakeButton(FormatTryAgain(), BUTTON_TRY_AGAIN));
}

string CountryStatusHelper::FormatDownloadMap()
{
  size_t size;
  string units;
  FormatMapSize(m_accessor->GetMapSize(), units, size);
  return strings::Format(GetLocalizedString(DownloadMapButtonID), size, units);
}

string CountryStatusHelper::FormatDownloadMapRouting()
{
  size_t size;
  string units;
  FormatMapSize(m_accessor->GetMapSize() + m_accessor->GetRoutingSize(), units, size);
  return strings::Format(GetLocalizedString(DownloadMapRoutingButtonID), size, units);
}

string CountryStatusHelper::FormatInQueueMap()
{
  return strings::Format(GetLocalizedString(InQueueID), m_accessor->GetCurrentCountryName());
}

string CountryStatusHelper::FormatFailed()
{
  return strings::Format(GetLocalizedString(DownloadingFailedID), m_accessor->GetCurrentCountryName());
}

string CountryStatusHelper::FormatTryAgain()
{
  return GetLocalizedString(TryAgainButtonID);
}

}  // namespace gui
