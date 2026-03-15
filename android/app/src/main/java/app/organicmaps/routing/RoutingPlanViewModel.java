package app.organicmaps.routing;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import com.google.android.material.bottomsheet.BottomSheetBehavior;

public class RoutingPlanViewModel extends ViewModel
{
  private final MutableLiveData<Integer> mRoutingBottomDistanceToTop = new MutableLiveData<>();
  private final MutableLiveData<Boolean> showRoutingBottomsheet = new MutableLiveData<>();
  private final MutableLiveData<Boolean> isPlacePageactive = new MutableLiveData<>();
  private final MutableLiveData<Integer> bottomSheetState = new MutableLiveData<>(BottomSheetBehavior.STATE_COLLAPSED);

  public LiveData<Integer> getBottomSheetState()
  {
    return bottomSheetState;
  }

  public void setBottomSheetState(Integer state)
  {
    bottomSheetState.setValue(state);
  }

  public LiveData<Boolean> getIsPlacePageactive()
  {
    return isPlacePageactive;
  }

  public void setIsPlacePageactive(Boolean isPlacePageactive)
  {
    this.isPlacePageactive.setValue(isPlacePageactive);
  }
  public LiveData<Boolean> getShowRoutingBottomsheet()
  {
    return showRoutingBottomsheet;
  }

  public void setShowRoutingBottomsheet(Boolean showRoutingBottomsheet)
  {
    this.showRoutingBottomsheet.setValue(showRoutingBottomsheet);
  }

  public LiveData<Integer> getmRoutingBottomDistanceToTop()
  {
    return mRoutingBottomDistanceToTop;
  }

  public void setmRoutingBottomDistanceToTop(Integer routingBottomDistanceToTop)
  {
    mRoutingBottomDistanceToTop.setValue(routingBottomDistanceToTop);
  }
}
