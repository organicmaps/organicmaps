package app.organicmaps.routing;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import com.google.android.material.bottomsheet.BottomSheetBehavior;

public class RoutingPlanViewModel extends ViewModel
{
  private final MutableLiveData<Integer> mRoutingBottomDistanceToTop = new MutableLiveData<>();
  private final MutableLiveData<Boolean> mShowRoutingBottomsheet = new MutableLiveData<>();
  private final MutableLiveData<Boolean> isPlacePageactive = new MutableLiveData<>();
  private final MutableLiveData<Integer> menuUpdateTrigger = new MutableLiveData<>(0);
  private final MutableLiveData<int[]> buildProgress = new MutableLiveData<>();
  private final MutableLiveData<Integer> drivingOptionsCount = new MutableLiveData<>(0);
  private final MutableLiveData<Boolean> drivingOptionsErrorTrigger = new MutableLiveData<>();
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
    return isPlacePageactive;
  }

  public void setIsPlacePageActive(Boolean isPlacePageactive)
  {
    this.isPlacePageactive.setValue(isPlacePageactive);
  }

  public LiveData<Boolean> getShowRoutingBottomsheet()
  {
    return mShowRoutingBottomsheet;
  }

  public void setShowRoutingBottomsheet(Boolean showRoutingBottomsheet)
  {
    this.mShowRoutingBottomsheet.setValue(showRoutingBottomsheet);
  }

  public LiveData<Integer> getRoutingBottomDistanceToTop()
  {
    return mRoutingBottomDistanceToTop;
  }

  public void setRoutingBottomDistanceToTop(Integer routingBottomDistanceToTop)
  {
    mRoutingBottomDistanceToTop.setValue(routingBottomDistanceToTop);
  }

  public LiveData<Integer> getMenuUpdateTrigger()
  {
    return menuUpdateTrigger;
  }

  public void triggerMenuUpdate()
  {
    Integer current = menuUpdateTrigger.getValue();
    menuUpdateTrigger.setValue(current == null ? 1 : current + 1);
  }

  // This is a workaround to update the progress and will be removed when the routing types are made segmented buttons
  public LiveData<int[]> getBuildProgress()
  {
    return buildProgress;
  }

  public void setBuildProgress(int progress, int routerOrdinal)
  {
    buildProgress.setValue(new int[] {progress, routerOrdinal});
  }

  public LiveData<Integer> getDrivingOptionsCount()
  {
    return drivingOptionsCount;
  }

  public void setDrivingOptionsCount(Integer count)
  {
    drivingOptionsCount.setValue(count);
  }

  public LiveData<Boolean> getDrivingOptionsErrorTrigger()
  {
    return drivingOptionsErrorTrigger;
  }

  public void triggerDrivingOptionsError()
  {
    drivingOptionsErrorTrigger.setValue(true);
  }
}
