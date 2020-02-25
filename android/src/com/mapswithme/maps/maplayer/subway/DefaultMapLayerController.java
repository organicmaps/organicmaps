package com.mapswithme.maps.maplayer.subway;

import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import com.mapswithme.maps.maplayer.MapLayerController;
import com.mapswithme.util.UiUtils;

public class DefaultMapLayerController implements MapLayerController
{
  @NonNull
  private final View mLayerView;

  public DefaultMapLayerController(@NonNull View subwayBtn)
  {
    mLayerView = subwayBtn;
    UiUtils.addStatusBarOffset(mLayerView);
  }

  @Override
  public void turnOn()
  {
    mLayerView.setSelected(true);
  }

  @Override
  public void turnOff()
  {
    mLayerView.setSelected(false);
  }

  @Override
  public void show()
  {
    UiUtils.show(mLayerView);
  }

  @Override
  public void showImmediately()
  {
    UiUtils.show(mLayerView);
  }

  @Override
  public void hide()
  {
    UiUtils.hide(mLayerView);
  }

  @Override
  public void hideImmediately()
  {
    UiUtils.hide(mLayerView);
  }

  @Override
  public void adjust(int offsetX, int offsetY)
  {
    ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) mLayerView.getLayoutParams();
    params.setMargins(offsetX, offsetY, 0, 0);
    mLayerView.setLayoutParams(params);
  }

  @Override
  public void attachCore()
  {
    /* Do nothing by default */
  }

  @Override
  public void detachCore()
  {
    /* Do nothing by default */
  }
}
