package com.mapswithme.maps.maplayer;

import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

import com.mapswithme.maps.maplayer.subway.SubwayMapLayerController;
import com.mapswithme.maps.maplayer.traffic.widget.TrafficButton;
import com.mapswithme.maps.maplayer.traffic.widget.TrafficButtonController;

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
    OpenBottomDialogClickListener listener = new OpenBottomDialogClickListener();
    mActivity = activity;
    mChildrenEntries = createEntries(traffic, subway, activity, listener);
    mMasterEntry = getCurrentLayer();
    toggleMode(mMasterEntry.mMode);
  }

  @NonNull
  private static Collection<ControllerAndMode> createEntries(@NonNull TrafficButton traffic,
                                                             @NonNull View subway,
                                                             @NonNull AppCompatActivity activity,
                                                             @NonNull OpenBottomDialogClickListener dialogClickListener)
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

  public void toggleMode(@NonNull Mode mode, boolean animate)
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
    mMasterEntry.mController.hideImmediately();
    mMasterEntry = mChildrenEntries.iterator().next();
    mMasterEntry.mController.showImmediately();
  }

  public void applyLastActiveMode()
  {
    toggleMode(mMasterEntry.mMode, true);
  }

  @Override
  public void attachCore()
  {
    for (ControllerAndMode each : mChildrenEntries)
    {
      each.mController.attachCore();
    }
  }

  @Override
  public void detachCore()
  {
    for (ControllerAndMode each : mChildrenEntries)
    {
      each.mController.detachCore();
    }
  }

  private void setMasterController(@NonNull Mode mode)
  {
    for (ControllerAndMode each : mChildrenEntries)
    {
      if (each.mMode == mode)
      {
        mMasterEntry = each;
      }
      else
      {
        each.mController.hideImmediately();
        each.mMode.setEnabled(mActivity, false);
      }
    }
  }

  private void showMasterController(boolean animate)
  {
    if (animate)
      mMasterEntry.mController.show();
    else
      mMasterEntry.mController.showImmediately();
  }

  @NonNull
  private ControllerAndMode getCurrentLayer()
  {
    for (ControllerAndMode each : mChildrenEntries)
    {
      if (each.mMode.isEnabled(mActivity))
        return each;
    }

    return mChildrenEntries.iterator().next();
  }

  @Override
  public void turnOn()
  {
    mMasterEntry.mController.turnOn();
    mMasterEntry.mMode.setEnabled(mActivity, true);
  }

  @Override
  public void turnOff()
  {
    mMasterEntry.mController.turnOff();
    mMasterEntry.mMode.setEnabled(mActivity, false);
  }

  @Override
  public void show()
  {
    mMasterEntry.mController.show();
  }

  @Override
  public void showImmediately()
  {
    mMasterEntry.mController.showImmediately();
  }

  @Override
  public void hide()
  {
    mMasterEntry.mController.hide();
  }

  @Override
  public void hideImmediately()
  {
    mMasterEntry.mController.hideImmediately();
  }

  @Override
  public void adjust(int offsetX, int offsetY)
  {
    mMasterEntry.mController.adjust(offsetX, offsetY);
  }

  private void showDialog()
  {
    ToggleMapLayerDialog.show(mActivity);
  }

  public void turnOn(@NonNull Mode mode)
  {
    ControllerAndMode entry = findModeMapLayerController(mode);
    entry.mMode.setEnabled(mActivity, true);
    entry.mController.turnOn();
    entry.mController.showImmediately();
  }

  public void turnOff(@NonNull Mode mode)
  {
    ControllerAndMode entry = findModeMapLayerController(mode);
    entry.mMode.setEnabled(mActivity, false);
    entry.mController.turnOff();
    entry.mController.hideImmediately();
    turnInitialMode();
  }

  @NonNull
  private ControllerAndMode findModeMapLayerController(@NonNull Mode mode)
  {
    for (ControllerAndMode each : mChildrenEntries)
    {
      if (each.mMode == mode)
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
      return mMode == that.mMode;
    }

    @Override
    public int hashCode()
    {
      return mMode.hashCode();
    }
  }

  private class OpenBottomDialogClickListener implements View.OnClickListener
  {
    @Override
    public void onClick(View v)
    {
      if (mMasterEntry.mMode.isEnabled(mActivity))
      {
        turnOff();
        toggleMode(getCurrentLayer().mMode);
      }
      else
      {
        showDialog();
      }
    }
  }
}
