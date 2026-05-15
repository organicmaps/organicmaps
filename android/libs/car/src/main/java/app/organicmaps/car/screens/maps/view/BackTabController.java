package app.organicmaps.car.screens.maps.view;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.car.app.Screen;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.TabContents;
import app.organicmaps.car.R;
import app.organicmaps.sdk.car.screens.BaseScreen;

final class BackTabController extends TabController
{
  private static final String CONTENT_ID = "back";

  public BackTabController(@NonNull BaseScreen parentScreen)
  {
    super(parentScreen);
  }

  @Override
  @StringRes
  public int getTabName()
  {
    return R.string.back;
  }

  @Override
  @NonNull
  public CarIcon getTabIcon()
  {
    return CarIcon.BACK;
  }

  @Override
  @NonNull
  public String getTabContentId()
  {
    return CONTENT_ID;
  }

  @Override
  @NonNull
  public TabContents getTabContents()
  {
    throw new IllegalStateException("Should not be called");
  }

  @Override
  public boolean onBackPressed()
  {
    final Screen screen = getScreen();
    if (screen != null)
      screen.finish();
    return true;
  }
}
