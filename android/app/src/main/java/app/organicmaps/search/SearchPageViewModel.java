package app.organicmaps.search;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
public class SearchPageViewModel extends ViewModel
{
  private final MutableLiveData<Integer> mSearchPageDistanceToTop = new MutableLiveData<>();
  private final MutableLiveData<Integer> mExpandedOffset = new MutableLiveData<>(0);
  private final MutableLiveData<Integer> mSearchPageWidth = new MutableLiveData<>();
  private final MutableLiveData<Integer> mHistoryRefreshRequest = new MutableLiveData<>(0);
  // Stores the BottomSheet's last stable non-hidden state (set from onStateChanged). Used to restore
  // the sheet after the place page temporarily hides it. Disabling search — including the user
  // dragging the sheet to hidden, which triggers the hidden callback — resets it to STATE_HIDDEN.
  private final MutableLiveData<Integer> mSearchPageLastState = new MutableLiveData<>(BottomSheetBehavior.STATE_HIDDEN);
  private final MutableLiveData<Boolean> mSearchEnabled = new MutableLiveData<>();
  private final MutableLiveData<Integer> mToolbarHeight = new MutableLiveData<>();
  private boolean mKeyboardVisible = false;

  // Pending search to run, set by the entry point before enabling search and consumed once by the
  // search fragment. Null when there is nothing new to start.
  @Nullable
  private SearchRequest mPendingRequest;

  private boolean mHiddenByPlacePage = false;

  @NonNull
  public LiveData<Integer> getSearchPageDistanceToTop()
  {
    return mSearchPageDistanceToTop;
  }

  @NonNull
  public LiveData<Integer> getHistoryRefreshRequest()
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
  public LiveData<Integer> getExpandedOffset()
  {
    return mExpandedOffset;
  }

  public void setExpandedOffset(int offset)
  {
    mExpandedOffset.setValue(offset);
  }

  @NonNull
  public LiveData<Integer> getSearchPageWidth()
  {
    return mSearchPageWidth;
  }

  public void setSearchPageWidth(int width)
  {
    mSearchPageWidth.setValue(width);
  }

  @NonNull
  public LiveData<Integer> getSearchPageLastState()
  {
    return mSearchPageLastState;
  }

  public void setSearchPageLastState(@BottomSheetBehavior.State int state)
  {
    // Ignore transient states: BottomSheetBehavior.setState() rejects DRAGGING/SETTLING, so storing
    // one would crash when the sheet is later restored from it.
    if (state == BottomSheetBehavior.STATE_DRAGGING || state == BottomSheetBehavior.STATE_SETTLING)
      return;
    mSearchPageLastState.setValue(state);
  }

  @NonNull
  public LiveData<Boolean> getSearchEnabled()
  {
    return mSearchEnabled;
  }

  @Nullable
  public SearchRequest getPendingRequest()
  {
    return mPendingRequest;
  }

  public void clearPendingRequest()
  {
    mPendingRequest = null;
  }

  public void setSearchEnabled(boolean enabled, @Nullable SearchRequest request)
  {
    // Store the pending request before firing LiveData so observers read it synchronously.
    mPendingRequest = enabled ? request : null;
    mHiddenByPlacePage = false;
    if (!enabled)
    {
      mSearchPageLastState.setValue(BottomSheetBehavior.STATE_HIDDEN);
      mKeyboardVisible = false;
    }
    mSearchEnabled.setValue(enabled);
  }

  @NonNull
  public LiveData<Integer> getToolbarHeight()
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
