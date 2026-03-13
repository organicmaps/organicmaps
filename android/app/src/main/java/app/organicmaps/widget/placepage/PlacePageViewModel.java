package app.organicmaps.widget.placepage;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import app.organicmaps.sdk.bookmarks.data.Bookmark;
import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.Track;
import java.util.List;

public class PlacePageViewModel extends ViewModel
{
  private final MutableLiveData<List<PlacePageButtons.ButtonType>> mCurrentButtons = new MutableLiveData<>();
  private final MutableLiveData<MapObject> mMapObject = new MutableLiveData<>();
  private final MutableLiveData<Integer> mPlacePageWidth = new MutableLiveData<>();
  private final MutableLiveData<Integer> mPlacePageDistanceToTop = new MutableLiveData<>();
  public boolean isAlertDialogShowing = false;
  // NEW: Store the last place page state
  private int mLastPlacePageState = BottomSheetBehavior.STATE_COLLAPSED;
  public LiveData<List<PlacePageButtons.ButtonType>> getCurrentButtons()
  {
    return mCurrentButtons;
  }

  public void setCurrentButtons(List<PlacePageButtons.ButtonType> buttons)
  {
    mCurrentButtons.setValue(buttons);
  }

  public MutableLiveData<MapObject> getMapObject()
  {
    return mMapObject;
  }

  public void setMapObject(MapObject mapObject)
  {
    mMapObject.setValue(mapObject);
  }

  public MutableLiveData<Integer> getPlacePageWidth()
  {
    return mPlacePageWidth;
  }

  public void setPlacePageWidth(int width)
  {
    mPlacePageWidth.setValue(width);
  }

  public MutableLiveData<Integer> getPlacePageDistanceToTop()
  {
    return mPlacePageDistanceToTop;
  }

  public void setPlacePageDistanceToTop(int top)
  {
    mPlacePageDistanceToTop.setValue(top);
  }
  // NEW: Methods to manage last place page state
  public int getLastPlacePageState()
  {
    return mLastPlacePageState;
  }

  public void setLastPlacePageState(int state)
  {
    mLastPlacePageState = state;
  }
}
