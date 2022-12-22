package app.organicmaps.car.util;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.ScreenManager;

public class OnBackPressedCallback extends androidx.activity.OnBackPressedCallback
{
  public interface Callback
  {
    void onBackPressed();
  }

  private final ScreenManager mScreenManager;
  private final Callback mCallback;

  public OnBackPressedCallback(@NonNull CarContext carContext, @NonNull Callback callback)
  {
    super(true);
    mScreenManager = carContext.getCarService(ScreenManager.class);
    mCallback = callback;
  }

  @Override
  public void handleOnBackPressed()
  {
    mCallback.onBackPressed();
    mScreenManager.pop();
  }
}
