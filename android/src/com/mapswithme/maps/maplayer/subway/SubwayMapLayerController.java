package com.mapswithme.maps.maplayer.subway;

import android.support.annotation.NonNull;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.maplayer.MapLayerController;
import com.mapswithme.util.Animations;
import com.mapswithme.util.UiUtils;

public class SubwayMapLayerController implements MapLayerController
{
  @NonNull
  private final View mSubwayBtn;

  public SubwayMapLayerController(@NonNull View subwayBtn)
  {
    mSubwayBtn = subwayBtn;
    UiUtils.addStatusBarOffset(mSubwayBtn);
  }

  @Override
  public void turnOn()
  {
    mSubwayBtn.setSelected(true);
  }

  @Override
  public void turnOff()
  {
    mSubwayBtn.setSelected(false);
  }

  @Override
  public void show()
  {
    Animations.appearSliding(mSubwayBtn, Animations.LEFT, null);
  }

  @Override
  public void showImmediately()
  {
    mSubwayBtn.setVisibility(View.VISIBLE);
  }

  @Override
  public void hide()
  {
    Animations.disappearSliding(mSubwayBtn, Animations.LEFT, null);
  }

  @Override
  public void hideImmediately()
  {
    mSubwayBtn.setVisibility(View.GONE);
  }

  @Override
  public void adjust(int offsetX, int offsetY)
  {
    ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) mSubwayBtn.getLayoutParams();
    params.setMargins(offsetX, offsetY, 0, 0);
    mSubwayBtn.setLayoutParams(params);
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
