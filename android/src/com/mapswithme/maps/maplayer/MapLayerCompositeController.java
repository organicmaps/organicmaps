package com.mapswithme.maps.maplayer;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

import com.mapswithme.maps.maplayer.subway.SubwayMapLayerController;
import com.mapswithme.maps.maplayer.traffic.widget.TrafficButton;
import com.mapswithme.maps.maplayer.traffic.widget.TrafficButtonController;
import com.mapswithme.maps.tips.TipsApi;
import com.mapswithme.maps.tips.TipsClickListener;
import com.mapswithme.util.InputUtils;

import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.Set;

public class MapLayerCompositeController implements MapLayerController
{
  @NonNull
  private final AppCompatActivity mActivity;
  @NonNull
  private final Collection<ControllerAndMode> mChildrenEntries;
  @NonNull
  private ControllerAndMode mMasterEntry;

  public MapLayerCompositeController(@NonNull TrafficButton traffic, @NonNull View subway,
                                     @NonNull AppCompatActivity activity)
  {
    View.OnClickListener listener = new OpenBottomDialogClickListener(activity, TipsApi.MAP_LAYERS);
    mActivity = activity;
    mChildrenEntries = createEntries(traffic, subway, activity, listener);
    mMasterEntry = getCurrentLayer();
    toggleMode(mMasterEntry.getMode());
  }

  @NonNull
  private static Collection<ControllerAndMode> createEntries(@NonNull TrafficButton traffic,
                                                             @NonNull View subway,
                                                             @NonNull AppCompatActivity activity,
                                                             @NonNull View.OnClickListener dialogClickListener)
  {
    traffic.setOnclickListener(dialogClickListener);
    TrafficButtonController trafficButtonController = new TrafficButtonController(traffic,
                                                                                  activity);
    subway.setOnClickListener(dialogClickListener);
    SubwayMapLayerController subwayMapLayerController = new SubwayMapLayerController(subway);

    ControllerAndMode subwayEntry = new ControllerAndMode(Mode.SUBWAY, subwayMapLayerController);
    ControllerAndMode trafficEntry = new ControllerAndMode(Mode.TRAFFIC, trafficButtonController);
    Set<ControllerAndMode> entries = new LinkedHashSet<>();
    entries.add(subwayEntry);
    entries.add(trafficEntry);
    return Collections.unmodifiableSet(entries);
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
    mMasterEntry.getController().hideImmediately();
    mMasterEntry = mChildrenEntries.iterator().next();
    mMasterEntry.getController().showImmediately();
  }

  public void applyLastActiveMode()
  {
    toggleMode(mMasterEntry.getMode(), true);
  }

  @Override
  public void attachCore()
  {
    for (ControllerAndMode each : mChildrenEntries)
    {
      each.getController().attachCore();
    }
  }

  @Override
  public void detachCore()
  {
    for (ControllerAndMode each : mChildrenEntries)
    {
      each.getController().detachCore();
    }
  }

  private void setMasterController(@NonNull Mode mode)
  {
    for (ControllerAndMode each : mChildrenEntries)
    {
      if (each.getMode() == mode)
      {
        mMasterEntry = each;
      }
      else
      {
        each.getController().hideImmediately();
        each.getMode().setEnabled(mActivity, false);
      }
    }
  }

  private void showMasterController(boolean animate)
  {
    if (animate)
      mMasterEntry.getController().show();
    else
      mMasterEntry.getController().showImmediately();
  }

  @NonNull
  private ControllerAndMode getCurrentLayer()
  {
    for (ControllerAndMode each : mChildrenEntries)
    {
      if (each.getMode().isEnabled(mActivity))
        return each;
    }

    return mChildrenEntries.iterator().next();
  }

  @Override
  public void turnOn()
  {
    mMasterEntry.getController().turnOn();
    mMasterEntry.getMode().setEnabled(mActivity, true);
  }

  @Override
  public void turnOff()
  {
    mMasterEntry.getController().turnOff();
    mMasterEntry.getMode().setEnabled(mActivity, false);
  }

  @Override
  public void show()
  {
    mMasterEntry.getController().show();
  }

  @Override
  public void showImmediately()
  {
    mMasterEntry.getController().showImmediately();
  }

  @Override
  public void hide()
  {
    mMasterEntry.getController().hide();
  }

  @Override
  public void hideImmediately()
  {
    mMasterEntry.getController().hideImmediately();
  }

  @Override
  public void adjust(int offsetX, int offsetY)
  {
    for(ControllerAndMode controllerAndMode: mChildrenEntries)
      controllerAndMode.getController().adjust(offsetX, offsetY);
  }

  private void showDialog()
  {
    ToggleMapLayerDialog.show(mActivity);
  }

  public void turnOn(@NonNull Mode mode)
  {
    ControllerAndMode entry = findModeMapLayerController(mode);
    entry.getMode().setEnabled(mActivity, true);
    entry.getController().turnOn();
    entry.getController().showImmediately();
  }

  public void turnOff(@NonNull Mode mode)
  {
    ControllerAndMode entry = findModeMapLayerController(mode);
    entry.getMode().setEnabled(mActivity, false);
    entry.getController().turnOff();
    entry.getController().hideImmediately();
    turnInitialMode();
  }

  @NonNull
  private ControllerAndMode findModeMapLayerController(@NonNull Mode mode)
  {
    for (ControllerAndMode each : mChildrenEntries)
    {
      if (each.getMode() == mode)
        return each;
    }

    throw new IllegalArgumentException("Mode not found : " + mode);
  }

  private static class ControllerAndMode
  {
    @NonNull
    private final Mode mMode;
    @NonNull
    private final MapLayerController mController;

    ControllerAndMode(@NonNull Mode mode, @NonNull MapLayerController controller)
    {
      mMode = mode;
      mController = controller;
    }

    @Override
    public boolean equals(Object o)
    {
      if (this == o) return true;
      if (o == null || getClass() != o.getClass()) return false;
      ControllerAndMode that = (ControllerAndMode) o;
      return mMode == that.getMode();
    }

    @Override
    public int hashCode()
    {
      return mMode.hashCode();
    }

    @NonNull
    MapLayerController getController()
    {
      return mController;
    }

    @NonNull
    Mode getMode()
    {
      return mMode;
    }
  }

  private class OpenBottomDialogClickListener extends TipsClickListener
  {
    OpenBottomDialogClickListener(@NonNull Activity activity, @NonNull TipsApi provider)
    {
      super(activity, provider);
    }

    @Override
    public void onProcessClick(@NonNull View view)
    {
      if (mMasterEntry.getMode().isEnabled(mActivity))
      {
        turnOff();
        toggleMode(getCurrentLayer().getMode());
      }
      else
      {
        InputUtils.hideKeyboard(mActivity.getWindow().getDecorView());
        showDialog();
      }
    }
  }
}
