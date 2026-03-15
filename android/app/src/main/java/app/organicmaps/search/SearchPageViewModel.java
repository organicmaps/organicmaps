package app.organicmaps.search;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
public class SearchPageViewModel extends ViewModel
{
  private final MutableLiveData<Integer> mSearchPageDistanceToTop = new MutableLiveData<>();
  private final MutableLiveData<Integer> mHistoryRefreshRequest = new MutableLiveData<>(0);
  // This `mSearchPageLastState` mutable variable stores the current state of the BottomSheet when it is not hidden.
  // When its hidden it contains the last state before hiding.
  private final MutableLiveData<Integer> mSearchPageLastState = new MutableLiveData<>(BottomSheetBehavior.STATE_HIDDEN);
  private final MutableLiveData<Boolean> mSearchEnabled = new MutableLiveData<>();
  private final MutableLiveData<Integer> mToolbarHeight = new MutableLiveData<>();
  private boolean mKeyboardVisible = false;
  private String mSearchQuery = null;

  @Nullable
  private String mInitialLocale = null;
  private boolean mInitialSearchOnMap = true;

  private boolean mHiddenByPlacePage = false;

  public MutableLiveData<Integer> getSearchPageDistanceToTop()
  {
    return mSearchPageDistanceToTop;
  }

  @NonNull
  public MutableLiveData<Integer> getHistoryRefreshRequest()
  {
    return mHistoryRefreshRequest;
  }

  public void notifyHistoryChanged()
  {
    Integer current = mHistoryRefreshRequest.getValue();
    mHistoryRefreshRequest.setValue(current == null ? 1 : current + 1);
  }

  public void setSearchPageDistanceToTop(int top)
  {
    mSearchPageDistanceToTop.setValue(top);
  }

  @NonNull
  public MutableLiveData<Integer> getSearchPageLastState()
  {
    return mSearchPageLastState;
  }

  public void setSearchPageLastState(@BottomSheetBehavior.State int state)
  {
    if (state == BottomSheetBehavior.STATE_DRAGGING || state == BottomSheetBehavior.STATE_SETTLING)
      mSearchPageLastState.setValue(BottomSheetBehavior.STATE_HALF_EXPANDED);
    else
      mSearchPageLastState.setValue(state);
  }

  public MutableLiveData<Boolean> getSearchEnabled()
  {
    return mSearchEnabled;
  }

  public String getSearchQuery()
  {
    return mSearchQuery;
  }

  public void setSearchQuery(String query)
  {
    mSearchQuery = query;
  }

  public void setSearchEnabled(boolean enabled, @Nullable String query)
  {
    // Set query before firing LiveData so observers read the correct value synchronously.
    mSearchQuery = query;
    if (!enabled)
    {
      mHiddenByPlacePage = false;
      mSearchPageLastState.setValue(BottomSheetBehavior.STATE_HIDDEN);
      mKeyboardVisible = false;
    }
    else
    {
      mHiddenByPlacePage = false;
    }
    mSearchEnabled.setValue(enabled);
  }

  @NonNull
  public MutableLiveData<Integer> getToolbarHeight()
  {
    return mToolbarHeight;
  }

  public void setToolbarHeight(int height)
  {
    mToolbarHeight.setValue(height);
  }

  public boolean isKeyboardVisible()
  {
    return mKeyboardVisible;
  }

  public void setKeyboardVisible(boolean visible)
  {
    mKeyboardVisible = visible;
  }

  @Nullable
  public String getInitialLocale()
  {
    return mInitialLocale;
  }

  public void setInitialLocale(@Nullable String locale)
  {
    mInitialLocale = locale;
  }

  public boolean isInitialSearchOnMap()
  {
    return mInitialSearchOnMap;
  }

  public void setInitialSearchOnMap(boolean isSearchOnMap)
  {
    mInitialSearchOnMap = isSearchOnMap;
  }

  public boolean isHiddenByPlacePage()
  {
    return mHiddenByPlacePage;
  }

  public void setHiddenByPlacePage(boolean hidden)
  {
    if (mSearchEnabled.getValue() != null && !mSearchEnabled.getValue())
      return;
    mHiddenByPlacePage = hidden;
  }
}
