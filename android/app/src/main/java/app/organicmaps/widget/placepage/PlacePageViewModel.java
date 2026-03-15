package app.organicmaps.widget.placepage;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import java.util.List;

public class PlacePageViewModel extends ViewModel
{
  public enum PlacePageTab
  {
    SCHEDULE,
    DETAILS
  }

  private final MutableLiveData<List<PlacePageButtons.ButtonType>> mCurrentButtons = new MutableLiveData<>();
  private final MutableLiveData<MapObject> mMapObject = new MutableLiveData<>();
  private final MutableLiveData<Integer> mPlacePageWidth = new MutableLiveData<>();
  private final MutableLiveData<Integer> mPlacePageDistanceToTop = new MutableLiveData<>();
  private final MutableLiveData<PlacePageTab> mSelectedTab = new MutableLiveData<>(PlacePageTab.DETAILS);
  private final MutableLiveData<Boolean> mShowTabs = new MutableLiveData<>(false);
  public boolean isAlertDialogShowing = false;

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

  public LiveData<PlacePageTab> getSelectedTab()
  {
    return mSelectedTab;
  }

  public void setSelectedTab(PlacePageTab tab)
  {
    mSelectedTab.setValue(tab);
  }

  public LiveData<Boolean> getShowTabs()
  {
    return mShowTabs;
  }

  public void setShowTabs(boolean show)
  {
    mShowTabs.setValue(show);
  }
}
