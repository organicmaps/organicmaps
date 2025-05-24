package app.organicmaps.widget.modalsearch;

import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

public class ModalSearchViewModel extends ViewModel
{
  private final MutableLiveData<Integer> mModalSearchDistanceToTop = new MutableLiveData<>();
  private final MutableLiveData<Boolean> mModalSearchActive = new MutableLiveData<>();
  private final MutableLiveData<Boolean> mModalSearchSuspended = new MutableLiveData<>();
  private final MutableLiveData<Boolean> mModalSearchCollapsed = new MutableLiveData<>();
  private final MutableLiveData<Boolean> mIsQueryEmpty = new MutableLiveData<>(true);


  public MutableLiveData<Integer> getModalSearchDistanceToTop()
  {
    return mModalSearchDistanceToTop;
  }

  public void setModalSearchDistanceToTop(int top)
  {
    mModalSearchDistanceToTop.setValue(top);
  }

  public MutableLiveData<Boolean> getModalSearchActive()
  {
    return mModalSearchActive;
  }

  public void setModalSearchActive(Boolean active)
  {
    mModalSearchActive.setValue(active);
  }

  public MutableLiveData<Boolean> getModalSearchSuspended()
  {
    return mModalSearchSuspended;
  }

  public void setModalSearchSuspended(Boolean suspended)
  {
    mModalSearchSuspended.setValue(suspended);
  }

  public MutableLiveData<Boolean> getModalSearchCollapsed()
  {
    return mModalSearchCollapsed;
  }

  public void setModalSearchCollapsed(Boolean collapsed)
  {
    mModalSearchCollapsed.setValue(collapsed);
  }

  public MutableLiveData<Boolean> getIsQueryEmpty()
  {
    return mIsQueryEmpty;
  }

  public void setIsQueryEmpty(Boolean isQueryEmpty)
  {
    mIsQueryEmpty.setValue(isQueryEmpty);
  }
}
