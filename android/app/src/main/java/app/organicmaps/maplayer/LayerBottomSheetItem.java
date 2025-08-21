package app.organicmaps.maplayer;

import android.content.Context;
import android.view.View;
import androidx.annotation.AttrRes;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import app.organicmaps.R;
import app.organicmaps.adapter.OnItemClickListener;
import app.organicmaps.sdk.maplayer.Mode;
import app.organicmaps.util.ThemeUtils;

public class LayerBottomSheetItem
{
  @DrawableRes
  private final int mDrawableResId;
  @StringRes
  private final int mTitleResId;
  @NonNull
  private final Mode mMode;
  @NonNull
  private final OnItemClickListener<LayerBottomSheetItem> mItemClickListener;

  LayerBottomSheetItem(@DrawableRes int drawableResId, @StringRes int titleResId, @NonNull Mode mode,
                       @NonNull OnItemClickListener<LayerBottomSheetItem> itemClickListener)
  {
    mDrawableResId = drawableResId;
    mTitleResId = titleResId;
    mMode = mode;
    mItemClickListener = itemClickListener;
  }

  public static LayerBottomSheetItem create(@NonNull Context mContext, Mode mode,
                                            @NonNull OnItemClickListener<LayerBottomSheetItem> layerItemClickListener)
  {
    @AttrRes
    int drawableRes = 0;
    @StringRes
    int buttonTextResource = R.string.layers_title;
    switch (mode)
    {
    case OUTDOORS:
      drawableRes = R.attr.outdoorsMenuIcon;
      buttonTextResource = R.string.button_layer_outdoor;
      break;
    case SUBWAY:
      drawableRes = R.attr.subwayMenuIcon;
      buttonTextResource = R.string.button_layer_subway;
      break;
    case ISOLINES:
      drawableRes = R.attr.isolinesMenuIcon;
      buttonTextResource = R.string.button_layer_isolines;
      break;
    case TRAFFIC:
      drawableRes = R.attr.trafficMenuIcon;
      buttonTextResource = R.string.button_layer_traffic;
      break;
    case HIKING:
      drawableRes = R.attr.hikingMenuIcon;
      buttonTextResource = R.string.button_layer_hiking;
      break;
    case CYCLING:
      drawableRes = R.attr.cyclingMenuIcon;
      buttonTextResource = R.string.button_layer_cycling;
      break;
    }
    @DrawableRes
    final int drawableResId = ThemeUtils.getResource(mContext, drawableRes);
    return new LayerBottomSheetItem(drawableResId, buttonTextResource, mode, layerItemClickListener);
  }

  @NonNull
  public Mode getMode()
  {
    return mMode;
  }

  @DrawableRes
  public int getDrawable()
  {
    return mDrawableResId;
  }

  @StringRes
  public int getTitle()
  {
    return mTitleResId;
  }

  public void onClick(@NonNull View v, @NonNull LayerBottomSheetItem item)
  {
    mItemClickListener.onItemClick(v, item);
  }
}
