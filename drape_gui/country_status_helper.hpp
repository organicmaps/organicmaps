#pragma once

#include "drape/pointers.hpp"

#include "base/buffer_vector.hpp"

#include "std/atomic.hpp"
#include "std/string.hpp"

namespace storage { struct TIndex; }

namespace gui
{

class StorageAccessor;

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
    BUTTON_TYPE_MAP_ROUTING,
    BUTTON_TRY_AGAIN
  };

  struct Control
  {
    string m_label;
    EControlType m_type;
    EButtonType m_buttonType;
  };

  CountryStatusHelper();

  void SetStorageAccessor(ref_ptr<StorageAccessor> accessor);
  void SetCountryIndex(storage::TIndex const & index);

  void SetState(ECountryState state);
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
  string FormatDownloadMapRouting();
  string FormatInQueueMap();
  string FormatFailed();
  string FormatTryAgain();

private:
  atomic<ECountryState> m_state;
  buffer_vector<Control, 4> m_controls;
  ref_ptr<StorageAccessor> m_accessor;
};

}  // namespace gui
