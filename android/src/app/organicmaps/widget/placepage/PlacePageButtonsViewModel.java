package app.organicmaps.widget.placepage;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import java.util.List;

public class PlacePageButtonsViewModel extends ViewModel
{
  private final MutableLiveData<List<PlacePageButtons.ButtonType>> mCurrentButtons = new MutableLiveData<>();

  public LiveData<List<PlacePageButtons.ButtonType>> getCurrentButtons()
  {
    return mCurrentButtons;
  }

  public void setCurrentButtons(List<PlacePageButtons.ButtonType> buttons)
  {
    mCurrentButtons.setValue(buttons);
  }
}
