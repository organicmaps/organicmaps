package app.organicmaps.routing;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import app.organicmaps.util.SingleLiveEvent;
import com.google.android.material.bottomsheet.BottomSheetBehavior;

public class RoutingPlanViewModel extends ViewModel
{
  private final MutableLiveData<Integer> mRoutingBottomDistanceToTop = new MutableLiveData<>();
  private final MutableLiveData<Boolean> mShowRoutingBottomSheet = new MutableLiveData<>();
  private final MutableLiveData<Boolean> mIsPlacePageActive = new MutableLiveData<>();
  private final MutableLiveData<Boolean> mIsSearchActive = new MutableLiveData<>();
  private final MutableLiveData<Integer> mMenuUpdateTrigger = new MutableLiveData<>(0);
  private final MutableLiveData<int[]> mBuildProgress = new MutableLiveData<>();
  private final MutableLiveData<Integer> mDrivingOptionsCount = new MutableLiveData<>(0);
  private final SingleLiveEvent<Void> mDrivingOptionsError = new SingleLiveEvent<>();
  private int mBottomSheetState = BottomSheetBehavior.STATE_COLLAPSED;

  public int getBottomSheetState()
  {
    return mBottomSheetState;
  }

  public void setBottomSheetState(int state)
  {
    mBottomSheetState = state;
  }

  public LiveData<Boolean> getIsPlacePageActive()
  {
    return mIsPlacePageActive;
  }

  public void setIsPlacePageActive(boolean active)
  {
    mIsPlacePageActive.setValue(active);
  }

  public LiveData<Boolean> getIsSearchActive()
  {
    return mIsSearchActive;
  }

  public void setIsSearchActive(boolean active)
  {
    mIsSearchActive.setValue(active);
  }

  public LiveData<Boolean> getShowRoutingBottomSheet()
  {
    return mShowRoutingBottomSheet;
  }

  public void setShowRoutingBottomSheet(boolean show)
  {
    mShowRoutingBottomSheet.setValue(show);
  }

  public LiveData<Integer> getRoutingBottomDistanceToTop()
  {
    return mRoutingBottomDistanceToTop;
  }

  public void setRoutingBottomDistanceToTop(int distance)
  {
    mRoutingBottomDistanceToTop.setValue(distance);
  }

  public LiveData<Integer> getMenuUpdateTrigger()
  {
    return mMenuUpdateTrigger;
  }

  public void triggerMenuUpdate()
  {
    Integer current = mMenuUpdateTrigger.getValue();
    mMenuUpdateTrigger.setValue(current == null ? 1 : current + 1);
  }

  // This is a workaround to update the progress and will be removed when the routing types are made segmented buttons
  public LiveData<int[]> getBuildProgress()
  {
    return mBuildProgress;
  }

  public void setBuildProgress(int progress, int routerOrdinal)
  {
    mBuildProgress.setValue(new int[] {progress, routerOrdinal});
  }

  public LiveData<Integer> getDrivingOptionsCount()
  {
    return mDrivingOptionsCount;
  }

  public void setDrivingOptionsCount(int count)
  {
    mDrivingOptionsCount.setValue(count);
  }

  public LiveData<Void> getDrivingOptionsError()
  {
    return mDrivingOptionsError;
  }

  public void triggerDrivingOptionsError()
  {
    mDrivingOptionsError.call();
  }
}
