package com.mapswithme.maps.maplayer;

import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.mapswithme.util.UiUtils;

import java.util.List;

public class MapLayersController
{
  @NonNull
  private final AppCompatActivity mActivity;
  @NonNull
  private final List<Mode> mLayers;
  @NonNull
  private final View mLayersButton;
  @NonNull
  OnShowMenuListener mOnShowMenuListener;
  @NonNull
  private Mode mCurrentLayer;

  public MapLayersController(@NonNull View layersButton, @NonNull OnShowMenuListener onShowMenuListener, @NonNull AppCompatActivity activity)
  {
    mActivity = activity;
    mLayersButton = layersButton;
    mLayersButton.setOnClickListener(view -> onLayersButtonClick());
    mOnShowMenuListener = onShowMenuListener;
    mLayers = LayersUtils.getAvailableLayers();
    mCurrentLayer = getCurrentLayer();
    initMode();
  }

  private void initMode()
  {
    setEnabled(mCurrentLayer.isEnabled(mActivity));
    showButton();
  }

  @NonNull
  private Mode getCurrentLayer()
  {
    for (Mode each : mLayers)
    {
      if (each.isEnabled(mActivity))
        return each;
    }
    return mLayers.iterator().next();
  }

  private void setCurrentLayer(@NonNull Mode mode)
  {
    for (Mode each : mLayers)
    {
      if (each == mode)
        mCurrentLayer = each;
      else
        each.setEnabled(mActivity, false);
    }
  }

  private void setEnabled(boolean enabled)
  {
    mLayersButton.setSelected(enabled);
    mCurrentLayer.setEnabled(mActivity, enabled);
  }

  private void onLayersButtonClick()
  {
    if (mCurrentLayer.isEnabled(mActivity))
      setEnabled(false);
    else
      mOnShowMenuListener.onShow();
  }

  public void toggleMode(@NonNull Mode mode)
  {
    setCurrentLayer(mode);
    showButton();
    setEnabled(!mode.isEnabled(mActivity));
  }

  public void showButton()
  {
    UiUtils.show(mLayersButton);
  }

  public void hideButton()
  {
    UiUtils.hide(mLayersButton);
  }

  public void adjust(int offsetX, int offsetY)
  {
    ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) mLayersButton.getLayoutParams();
    params.setMargins(offsetX, offsetY, 0, 0);
    mLayersButton.setLayoutParams(params);
  }

  public interface OnShowMenuListener
  {
    void onShow();
  }
}
