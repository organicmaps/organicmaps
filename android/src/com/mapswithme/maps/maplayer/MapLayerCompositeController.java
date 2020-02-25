package com.mapswithme.maps.maplayer;

import android.app.Activity;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import com.mapswithme.maps.maplayer.subway.DefaultMapLayerController;
import com.mapswithme.maps.maplayer.traffic.widget.TrafficButton;
import com.mapswithme.maps.maplayer.traffic.widget.TrafficButtonController;
import com.mapswithme.maps.tips.Tutorial;
import com.mapswithme.maps.tips.TutorialClickListener;
import com.mapswithme.util.InputUtils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class MapLayerCompositeController implements MapLayerController
{
  @NonNull
  private final AppCompatActivity mActivity;
  @NonNull
  private final List<ControllerAndMode> mChildrenEntries;
  @NonNull
  private ControllerAndMode mMasterEntry;
  @NonNull
  private final TutorialClickListener mOpenBottomDialogClickListener;

  public MapLayerCompositeController(@NonNull TrafficButton traffic, @NonNull View subway,
                                     @NonNull View isoLines, @NonNull AppCompatActivity activity)
  {
    mOpenBottomDialogClickListener = new OpenBottomDialogClickListener(activity);
    mActivity = activity;
    mChildrenEntries = createEntries(traffic, subway, isoLines, activity, mOpenBottomDialogClickListener);
    mMasterEntry = getCurrentLayer();
    toggleMode(mMasterEntry.getMode());
  }

  @NonNull
  private static List<ControllerAndMode> createEntries(@NonNull TrafficButton traffic,
                                                             @NonNull View subway,
                                                             @NonNull View isoLinesView,
                                                             @NonNull AppCompatActivity activity,
                                                             @NonNull View.OnClickListener dialogClickListener)
  {
    traffic.setOnclickListener(dialogClickListener);
    TrafficButtonController trafficButtonController = new TrafficButtonController(traffic,
                                                                                  activity);
    subway.setOnClickListener(dialogClickListener);
    DefaultMapLayerController subwayMapLayerController = new DefaultMapLayerController(subway);

    isoLinesView.setOnClickListener(dialogClickListener);
    DefaultMapLayerController isoLinesController = new DefaultMapLayerController(isoLinesView);

    ControllerAndMode subwayEntry = new ControllerAndMode(Mode.SUBWAY, Tutorial.SUBWAY,
                                                          subwayMapLayerController);
    ControllerAndMode trafficEntry = new ControllerAndMode(Mode.TRAFFIC, null,
                                                           trafficButtonController);
    ControllerAndMode isoLineEntry = new ControllerAndMode(Mode.ISOLINES, Tutorial.ISOLINES,
                                                           isoLinesController);

    List<ControllerAndMode> entries = new ArrayList<>();
    entries.add(subwayEntry);
    entries.add(isoLineEntry);
    entries.add(trafficEntry);

    return entries;
  }

  public void setTutorial(@NonNull Tutorial tutorial)
  {
    mOpenBottomDialogClickListener.setTutorial(tutorial);

    // The sorting is needed to put the controller mode corresponding to the specified tutorial
    // at the first place in the list. It allows to enable the map layer ignoring the opening the
    // bottom dialog when user taps on the pulsating map layer button.
    Collections.sort(mChildrenEntries, (lhs, rhs) ->
    {
      if (tutorial.equals(lhs.getTutorial()))
        return -1;
      if (tutorial.equals(rhs.getTutorial()))
        return 1;
      return 0;
    });
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
    @Nullable
    private final Tutorial mTutorial;
    @NonNull
    private final MapLayerController mController;

    ControllerAndMode(@NonNull Mode mode, @Nullable Tutorial tutorial,
                      @NonNull MapLayerController controller)
    {
      mMode = mode;
      mTutorial = tutorial;
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

    @Nullable
    Tutorial getTutorial()
    {
      return mTutorial;
    }
  }

  private class OpenBottomDialogClickListener extends TutorialClickListener
  {
    OpenBottomDialogClickListener(@NonNull Activity activity)
    {
      super(activity);
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
