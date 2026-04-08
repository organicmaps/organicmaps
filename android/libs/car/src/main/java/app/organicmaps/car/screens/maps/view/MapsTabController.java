package app.organicmaps.car.screens.maps.view;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.car.app.constraints.ConstraintManager;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.TabContents;
import androidx.car.app.model.Template;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.sdk.car.screens.BaseScreen;

/**
 * Generic tab controller backed by a {@link MapsViewer}.
 * All three map tabs (All, Mine, Updatable) use this implementation,
 * differing only in their content ID, label, icon and data type.
 */
class MapsTabController extends TabController
{
  @NonNull
  private final String mContentId;
  @StringRes
  private final int mTabNameRes;
  @NonNull
  private final CarIcon mTabIcon;
  @NonNull
  private final MapsViewer mMapsViewer;

  MapsTabController(@NonNull BaseScreen parentScreen, @NonNull String contentId, @StringRes int tabNameRes,
                    @DrawableRes int iconRes, @NonNull MapsProvider.Type type)
  {
    super(parentScreen);
    mContentId = contentId;
    mTabNameRes = tabNameRes;
    final int maxRowsAllowed = parentScreen.getCarContext()
                                   .getCarService(ConstraintManager.class)
                                   .getContentLimit(ConstraintManager.CONTENT_LIMIT_TYPE_LIST);
    mTabIcon = new CarIcon.Builder(IconCompat.createWithResource(parentScreen.getCarContext(), iconRes)).build();
    mMapsViewer = new MapsViewer(parentScreen.getCarContext(), parentScreen.getOrganicMapsContext(), maxRowsAllowed,
                                 type, this::invalidate);
  }

  @Override
  @StringRes
  public int getTabName()
  {
    return mTabNameRes;
  }

  @Override
  @NonNull
  public CarIcon getTabIcon()
  {
    return mTabIcon;
  }

  @Override
  @NonNull
  public String getTabContentId()
  {
    return mContentId;
  }

  @Override
  @NonNull
  public TabContents getTabContents()
  {
    final Template template = mMapsViewer.buildTemplate();
    return new TabContents.Builder(template).build();
  }

  @Override
  public boolean onBackPressed()
  {
    return mMapsViewer.onBackPressed();
  }

  @Override
  void refreshCurrentFolder()
  {
    mMapsViewer.refreshCurrentFolder();
  }

  @Override
  void setOnMapChangedCallback(@NonNull Runnable callback)
  {
    mMapsViewer.setOnMapChangedCallback(callback);
  }
}
