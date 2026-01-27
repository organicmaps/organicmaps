package app.organicmaps.search;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import app.organicmaps.sdk.search.SearchResult;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
public class SearchPageViewModel extends ViewModel
{
  private final MutableLiveData<Integer> mSearchPageDistanceToTop = new MutableLiveData<>();
  // This `mSearchPageLastState` mutable variable stores the current state of the BottomSheet when it is not hidden.
  // When its hidden it contains the last state before hiding.
  private final MutableLiveData<Integer> mSearchPageLastState =
      new MutableLiveData<>(BottomSheetBehavior.STATE_COLLAPSED);
  private final MutableLiveData<Boolean> mSearchEnabled = new MutableLiveData<>();
  private String mSearchQuery = null;
  @Nullable
  private SearchResult[] mLastResults = null;

  public MutableLiveData<Integer> getSearchPageDistanceToTop()
  {
    return mSearchPageDistanceToTop;
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
    mSearchEnabled.setValue(enabled);
    mSearchQuery = query;
  }

  @Nullable
  public SearchResult[] getLastResults()
  {
    return mLastResults;
  }

  public void setLastResults(@Nullable SearchResult[] results)
  {
    mLastResults = results;
  }
}
