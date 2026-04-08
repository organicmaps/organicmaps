package app.organicmaps.car.screens.maps.view;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.car.app.Screen;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.TabContents;
import app.organicmaps.sdk.car.screens.BaseScreen;
import java.lang.ref.WeakReference;

abstract class TabController
{
  @NonNull
  private final WeakReference<BaseScreen> mScreen;

  public TabController(@NonNull BaseScreen parentScreen)
  {
    mScreen = new WeakReference<>(parentScreen);
  }

  @Nullable
  protected BaseScreen getScreen()
  {
    return mScreen.get();
  }

  protected void invalidate()
  {
    final Screen screen = getScreen();
    if (screen != null)
      screen.invalidate();
  }

  @StringRes
  abstract int getTabName();

  @NonNull
  abstract CarIcon getTabIcon();

  @NonNull
  abstract String getTabContentId();

  /**
   * @return true if the back press was handled by the tab, false otherwise
   */
  abstract boolean onBackPressed();

  /** Refreshes the tab's data after a map change. No-op by default. */
  void refreshCurrentFolder() {}

  /** Sets a callback to invoke when a map download/removal completes. No-op by default. */
  void setOnMapChangedCallback(@NonNull Runnable callback) {}

  @NonNull
  abstract TabContents getTabContents();
}
