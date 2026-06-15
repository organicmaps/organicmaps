package app.organicmaps.search;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.SavedStateHandle;
import androidx.lifecycle.ViewModel;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
public class SearchPageViewModel extends ViewModel
{
  private static final String KEY_SEARCH_ACTIVE = "search_active";
  private static final String KEY_SEARCH_QUERY = "search_query";
  private static final String KEY_SEARCH_STATE = "search_sheet_state";
  private static final String KEY_SEARCH_IS_CATEGORY = "search_is_category";

  private final SavedStateHandle mSavedState;

  public SearchPageViewModel(@NonNull SavedStateHandle savedStateHandle)
  {
    mSavedState = savedStateHandle;
  }

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

  // Pending search to run, set by the entry point before enabling search and consumed once by the
  // search fragment. Null when there is nothing new to start.
  @Nullable
  private SearchRequest mPendingRequest;

  private boolean mCurrentToolbarIsCategory = false;

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
    // Only stable, non-hidden states are restorable: setState() rejects DRAGGING/SETTLING, and
    // HIDDEN means "no restorable state" (set explicitly via setSearchEnabled(false) instead).
    if (state != BottomSheetBehavior.STATE_EXPANDED && state != BottomSheetBehavior.STATE_HALF_EXPANDED
        && state != BottomSheetBehavior.STATE_COLLAPSED)
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

  /** Mirror of the live toolbar category flag — no other source records it for persistence. */
  public void setCurrentToolbarCategorical(boolean isCategory)
  {
    mCurrentToolbarIsCategory = isCategory;
  }

  public boolean isCurrentToolbarCategorical()
  {
    return mCurrentToolbarIsCategory;
  }

  public void setSearchEnabled(boolean enabled, @Nullable SearchRequest request)
  {
    // Store the pending request before firing LiveData so observers read it synchronously.
    mPendingRequest = enabled ? request : null;
    mHiddenByPlacePage = false;
    if (!enabled)
      mSearchPageLastState.setValue(BottomSheetBehavior.STATE_HIDDEN);
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

  /** Snapshots search state into SavedStateHandle so the framework can restore it after process death. */
  public void persistSearchState(boolean active, @NonNull String query, int sheetState, boolean isCategory)
  {
    mSavedState.set(KEY_SEARCH_ACTIVE, active);
    mSavedState.set(KEY_SEARCH_QUERY, query);
    mSavedState.set(KEY_SEARCH_STATE, sheetState);
    mSavedState.set(KEY_SEARCH_IS_CATEGORY, isCategory);
  }

  public boolean isSearchPersistedActive()
  {
    return Boolean.TRUE.equals(mSavedState.get(KEY_SEARCH_ACTIVE));
  }

  @NonNull
  public String getPersistedQuery()
  {
    String query = mSavedState.get(KEY_SEARCH_QUERY);
    return query != null ? query : "";
  }

  public int getPersistedSheetState()
  {
    Integer state = mSavedState.get(KEY_SEARCH_STATE);
    return state != null ? state : BottomSheetBehavior.STATE_EXPANDED;
  }

  public boolean getPersistedIsCategory()
  {
    return Boolean.TRUE.equals(mSavedState.get(KEY_SEARCH_IS_CATEGORY));
  }
}
