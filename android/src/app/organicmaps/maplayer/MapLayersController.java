package app.organicmaps.maplayer;

import android.app.Activity;
import android.view.ViewGroup;
import android.widget.ImageButton;

import androidx.annotation.NonNull;
import app.organicmaps.R;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.UiUtils;

import java.util.List;

public class MapLayersController
{
  @NonNull
  private final Activity mActivity;
  @NonNull
  private final List<Mode> mLayers;
  @NonNull
  private final ImageButton mLayersButton;
  @NonNull
  OnShowMenuListener mOnShowMenuListener;
  @NonNull
  private Mode mCurrentLayer;
  private final MapButtonsViewModel mMapButtonsViewModel;

  public MapLayersController(@NonNull ImageButton layersButton, @NonNull OnShowMenuListener onShowMenuListener, @NonNull Activity activity, MapButtonsViewModel mapButtonsViewModel)
  {
    mActivity = activity;
    mMapButtonsViewModel = mapButtonsViewModel;
    mLayersButton = layersButton;
    mLayersButton.setOnClickListener(view -> onLayersButtonClick());
    mOnShowMenuListener = onShowMenuListener;
    mLayers = LayersUtils.getAvailableLayers();
    mCurrentLayer = getCurrentLayer();
    // View model only expects a layer if it is active
    mMapButtonsViewModel.setMapLayerMode(mCurrentLayer.isEnabled(activity) ? mCurrentLayer : null);
    showButton(true);
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
    int drawable = R.drawable.ic_layers;
    if (enabled)
      drawable = R.drawable.ic_layers_clear;
    mLayersButton.setImageDrawable(Graphics.tint(mLayersButton.getContext(), drawable));
  }

  private void onLayersButtonClick()
  {
    if (mCurrentLayer.isEnabled(mActivity))
      mMapButtonsViewModel.setMapLayerMode(null);
    else
      mOnShowMenuListener.onShow();
  }

  public void disableModes()
  {
    setEnabled(false);
  }

  public void enableMode(@NonNull Mode mode)
  {
    setCurrentLayer(mode);
    showButton(true);
    setEnabled(true);
  }

  public void showButton(boolean show)
  {
    UiUtils.showIf(show, mLayersButton);
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
