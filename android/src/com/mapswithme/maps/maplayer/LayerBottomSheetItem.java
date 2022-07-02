package com.mapswithme.maps.maplayer;

import android.content.Context;
import android.view.View;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.OnItemClickListener;
import com.mapswithme.util.ThemeUtils;

public class LayerBottomSheetItem
{
  @DrawableRes
  private final int mEnabledStateDrawableResId;
  @DrawableRes
  private final int mDisabledStateDrawableResId;
  @StringRes
  private final int mTitleResId;
  @NonNull
  private final Mode mMode;
  @NonNull
  private final OnItemClickListener<LayerBottomSheetItem> mItemClickListener;

  LayerBottomSheetItem(@DrawableRes int enabledStateDrawableResId,
                       @DrawableRes int disabledStateDrawableResId,
                       @StringRes int titleResId,
                       @NonNull Mode mode,
                       @NonNull OnItemClickListener<LayerBottomSheetItem> itemClickListener)
  {
    mEnabledStateDrawableResId = enabledStateDrawableResId;
    mDisabledStateDrawableResId = disabledStateDrawableResId;
    mTitleResId = titleResId;
    mMode = mode;
    mItemClickListener = itemClickListener;
  }

  public static LayerBottomSheetItem create(@NonNull Context mContext, Mode mode, @NonNull OnItemClickListener<LayerBottomSheetItem> layerItemClickListener)
  {
    int disabledResource = 0;
    int enabledResource = 0;
    int buttonTextResource = R.string.layers_title;
    switch (mode)
    {
      case SUBWAY:
        disabledResource = R.attr.subwayMenuDisabled;
        enabledResource = R.attr.subwayMenuEnabled;
        buttonTextResource = R.string.button_layer_subway;
        break;
      case ISOLINES:
        disabledResource = R.attr.isoLinesMenuDisabled;
        enabledResource = R.attr.isoLinesMenuEnabled;
        buttonTextResource = R.string.button_layer_isolines;
        break;
      case TRAFFIC:
        disabledResource = R.attr.trafficMenuDisabled;
        enabledResource = R.attr.trafficMenuEnabled;
        buttonTextResource = R.string.button_layer_traffic;
        break;
    }
    int disabled = ThemeUtils.getResource(mContext, disabledResource);
    int enabled = ThemeUtils.getResource(mContext, enabledResource);
    return new LayerBottomSheetItem(enabled, disabled, buttonTextResource, mode, layerItemClickListener);
  }

  @NonNull
  public Mode getMode()
  {
    return mMode;
  }

  @DrawableRes
  public int getEnabledStateDrawable()
  {
    return mEnabledStateDrawableResId;
  }

  @DrawableRes
  public int getDisabledStateDrawable()
  {
    return mDisabledStateDrawableResId;
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
