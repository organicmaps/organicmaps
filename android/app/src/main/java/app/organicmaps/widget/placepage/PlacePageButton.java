package app.organicmaps.widget.placepage;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

public class PlacePageButton
{
  @StringRes
  private final int mTitleId;
  @DrawableRes
  private final int mIconId;
  @NonNull
  private final PlacePageButtons.ButtonType mButtonType;

  PlacePageButton(@StringRes int titleId, @DrawableRes int iconId, @NonNull PlacePageButtons.ButtonType buttonType)
  {
    mTitleId = titleId;
    mIconId = iconId;
    mButtonType = buttonType;
  }

  @StringRes
  public int getTitle()
  {
    return mTitleId;
  }

  @DrawableRes
  public int getIcon()
  {
    return mIconId;
  }

  @NonNull
  public PlacePageButtons.ButtonType getType()
  {
    return mButtonType;
  }
}
