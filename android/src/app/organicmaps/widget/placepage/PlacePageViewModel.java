package app.organicmaps.widget.placepage;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import app.organicmaps.bookmarks.data.MapObject;

import java.util.List;

public class PlacePageViewModel extends ViewModel
{
  private final MutableLiveData<List<PlacePageButtons.ButtonType>> mCurrentButtons = new MutableLiveData<>();
  private final MutableLiveData<MapObject> mMapObject = new MutableLiveData<>();

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
}
