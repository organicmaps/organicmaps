#pragma once

#include "drape/pointers.hpp"

#include "storage/index.hpp"
#include "storage/storage_defines.hpp"

#include "base/buffer_vector.hpp"

#include "std/atomic.hpp"
#include "std/string.hpp"

namespace gui
{

struct CountryInfo
{
  storage::TCountryId m_countryIndex = storage::kInvalidCountryId;
  storage::Status m_countryStatus = storage::Status::EUnknown;
  string m_currentCountryName;
  size_t m_mapSize = 0;
  size_t m_downloadProgress = 0;
  bool m_showMapSize = true;
};

class CountryStatusHelper
{
public:
  enum ECountryState
  {
    COUNTRY_STATE_EMPTY,
    COUNTRY_STATE_LOADED,
    COUNTRY_STATE_LOADING,
    COUNTRY_STATE_IN_QUEUE,
    COUNTRY_STATE_FAILED
  };

  enum EControlType
  {
    CONTROL_TYPE_LABEL,
    CONTROL_TYPE_BUTTON,
    CONTROL_TYPE_PROGRESS
  };

  enum EButtonType
  {
    BUTTON_TYPE_NOT_BUTTON,
    BUTTON_TYPE_MAP,
    BUTTON_TRY_AGAIN,
    BUTTON_CANCEL
  };

  struct Control
  {
    string m_label;
    EControlType m_type;
    EButtonType m_buttonType;
  };

  CountryStatusHelper();

  void SetCountryInfo(CountryInfo const & countryInfo);
  void Clear();

  storage::TCountryId GetCountryIndex() const;
  ECountryState GetState() const;
  /// CountryStatusHandle work on FrontendRenderer and call this function to check "is visible"
  /// or state has already changed.
  /// State changes from BackendRenderer thread, when recache operation started.
  /// In that moment no need to show old CountryStatus
  bool IsVisibleForState(ECountryState state) const;

  size_t GetComponentCount() const;
  Control const & GetControl(size_t index) const;
  static float GetControlMargin();

  static void GetProgressInfo(string & alphabet, size_t & maxLength);
  string GetProgressValue() const;

private:
  void FillControlsForState();
  void FillControlsForEmpty();
  void FillControlsForLoading();
  void FillControlsForInQueue();
  void FillControlsForFailed();

  string FormatDownloadMap();
  string FormatInQueueMap();
  string FormatFailed();
  string FormatTryAgain();
  string FormatCancel();

  void SetState(ECountryState state);

  ECountryState m_state;
  buffer_vector<Control, 4> m_controls;
  CountryInfo m_countryInfo;
};

}  // namespace gui
