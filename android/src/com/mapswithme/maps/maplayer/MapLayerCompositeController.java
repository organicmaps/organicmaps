package com.mapswithme.maps.maplayer;

import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.mapswithme.util.InputUtils;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.List;

public class MapLayerCompositeController implements MapLayerController
{
  @NonNull
  private final AppCompatActivity mActivity;
  @NonNull
  private final List<Mode> mLayers;
  @NonNull
  private Mode mCurrentLayer;
  @NonNull
  private final View mLayersButton;
  @NonNull
  OnShowMenuListener mOnshowMenuListener;

  public MapLayerCompositeController(@NonNull View layersButton, @NonNull OnShowMenuListener onshowMenuListener, @NonNull AppCompatActivity activity)
  {
    mActivity = activity;
    mLayersButton = layersButton;
    mLayersButton.setOnClickListener(view -> onLayersButtonClick());
    mOnshowMenuListener = onshowMenuListener;
    mLayers = new ArrayList<>();
    mLayers.add(Mode.SUBWAY);
    mLayers.add(Mode.ISOLINES);
    mCurrentLayer = getCurrentLayer();
    toggleMode(mCurrentLayer);
  }

  public void toggleMode(@NonNull Mode mode)
  {
    toggleMode(mode, false);
  }

  private void toggleMode(@NonNull Mode mode, boolean animate)
  {
    setMasterController(mode);
    showMasterController(animate);

    boolean enabled = mode.isEnabled(mActivity);
    if (enabled)
    {
      turnOn();
    }
    else
    {
      turnOff();
      turnInitialMode();
    }
  }

  private void turnInitialMode()
  {
    UiUtils.hide(mLayersButton);
    mCurrentLayer = mLayers.iterator().next();
    UiUtils.show(mLayersButton);
  }

  public void applyLastActiveMode()
  {
    toggleMode(mCurrentLayer, true);
  }

  @Override
  public void attachCore()
  {
    // Do nothing
  }

  @Override
  public void detachCore()
  {
    // Do nothing
  }

  private void setMasterController(@NonNull Mode mode)
  {
    for (Mode each : mLayers)
    {
      if (each == mode)
      {
        mCurrentLayer = each;
      }
      else
      {
        UiUtils.hide(mLayersButton);
        each.setEnabled(mActivity, false);
      }
    }
  }

  private void showMasterController(boolean animate)
  {
    UiUtils.show(mLayersButton);
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

  @Override
  public void turnOn()
  {
    mLayersButton.setSelected(true);
    mCurrentLayer.setEnabled(mActivity, true);
  }

  @Override
  public void turnOff()
  {
    mLayersButton.setSelected(false);
    mCurrentLayer.setEnabled(mActivity, false);
  }

  @Override
  public void show()
  {
    UiUtils.show(mLayersButton);
  }

  @Override
  public void showImmediately()
  {
    UiUtils.show(mLayersButton);
  }

  @Override
  public void hide()
  {
    UiUtils.hide(mLayersButton);
  }

  @Override
  public void hideImmediately()
  {
    UiUtils.hide(mLayersButton);
  }

  @Override
  public void adjust(int offsetX, int offsetY)
  {
    ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) mLayersButton.getLayoutParams();
    params.setMargins(offsetX, offsetY, 0, 0);
    mLayersButton.setLayoutParams(params);
  }

  public void turnOnView(@NonNull Mode mode)
  {
    setMasterController(mode);
    UiUtils.show(mLayersButton);
    mLayersButton.setSelected(true);
  }

  private void onLayersButtonClick()
  {
    if (mCurrentLayer.isEnabled(mActivity))
    {
      Mode mode = getCurrentLayer();
      turnOff();
      toggleMode(mode);
    } else
    {
      InputUtils.hideKeyboard(mActivity.getWindow().getDecorView());
      mOnshowMenuListener.onShow();
    }
  }

  public interface OnShowMenuListener
  {
    void onShow();
  }
}
