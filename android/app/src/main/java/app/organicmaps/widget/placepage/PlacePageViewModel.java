package app.organicmaps.widget.placepage;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
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

  // These silent method are used to update data of map object silently without triggering
  // the observer which refreshes everything and puts extra load  on device in calculation
  // which is not required
  public void modifyMapObjectPointSilently(ElevationInfo.Point point)
  {
    if (mMapObject.getValue() == null)
      return;
    mMapObject.getValue().setLat(point.getLatitude());
    mMapObject.getValue().setLon(point.getLongitude());
  }

  public void modifyMapObjectCategoryIdSilently(long categoryId)
  {
    if (mMapObject.getValue() == null)
      return;
    switch (PlacePageView.MapObjectType.getMapObjectType(mMapObject.getValue()))
    {
    case TRACK -> ((Track) mMapObject.getValue()).setCategoryId(categoryId);
    case BOOKMARK -> ((Bookmark) mMapObject.getValue()).setCategoryId(categoryId);
    }
  }

  public void modifyMapObjectColorSilently(int color)
  {
    if (mMapObject.getValue() == null)
      return;
    switch (PlacePageView.MapObjectType.getMapObjectType(mMapObject.getValue()))
    {
    case TRACK -> ((Track) mMapObject.getValue()).setColor(color);
    case BOOKMARK -> ((Bookmark) mMapObject.getValue()).setIconColor(color);
    }
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
}
