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
import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class MapLayerCompositeController implements MapLayerController
{
  @NonNull
  private final AppCompatActivity mActivity;
  @NonNull
  private final List<ControllerAndMode> mLayers;
  @NonNull
  private ControllerAndMode mCurrentLayer;
  @NonNull
  private final TutorialClickListener mOpenBottomDialogClickListener;

  public MapLayerCompositeController(@NonNull TrafficButton traffic, @NonNull View subway,
                                     @NonNull View isoLines, @NonNull View guides,
                                     @NonNull AppCompatActivity activity)
  {
    mOpenBottomDialogClickListener = new OpenBottomDialogClickListener(activity);
    mActivity = activity;
    mLayers = createLayers(traffic, subway, isoLines, guides, activity, mOpenBottomDialogClickListener);
    mCurrentLayer = getCurrentLayer();
    toggleMode(mCurrentLayer.getMode());
  }

  @NonNull
  private static List<ControllerAndMode> createLayers(@NonNull TrafficButton traffic,
                                                      @NonNull View subway,
                                                      @NonNull View isoLinesView,
                                                      @NonNull View guides,
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

    guides.setOnClickListener(dialogClickListener);
    DefaultMapLayerController guidesController = new DefaultMapLayerController(guides);

    ControllerAndMode subwayEntry = new ControllerAndMode(Mode.SUBWAY, Tutorial.SUBWAY,
                                                          subwayMapLayerController);
    ControllerAndMode trafficEntry = new ControllerAndMode(Mode.TRAFFIC, null,
                                                           trafficButtonController);
    ControllerAndMode isoLineEntry = new ControllerAndMode(Mode.ISOLINES, Tutorial.ISOLINES,
                                                           isoLinesController);
    ControllerAndMode guidesEntry = new ControllerAndMode(Mode.GUIDES, null,
                                                          guidesController);

    List<ControllerAndMode> entries = new ArrayList<>();
    entries.add(subwayEntry);
    entries.add(isoLineEntry);
    entries.add(trafficEntry);
    entries.add(guidesEntry);

    return entries;
  }

  public void setTutorial(@NonNull Tutorial tutorial)
  {
    mOpenBottomDialogClickListener.setTutorial(tutorial);

    // The sorting is needed to put the controller mode corresponding to the specified tutorial
    // at the first place in the list. It allows to enable the map layer ignoring the opening the
    // bottom dialog when user taps on the pulsating map layer button.
    Collections.sort(mLayers, (lhs, rhs) ->
    {
      if (tutorial.equals(lhs.getTutorial()))
        return -1;
      if (tutorial.equals(rhs.getTutorial()))
        return 1;
      return 0;
    });

    // The current layer must be updated after the layer controllers are sorted.
    mCurrentLayer = getCurrentLayer();
    toggleMode(mCurrentLayer.getMode());
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
    mCurrentLayer.getController().hideImmediately();
    mCurrentLayer = mLayers.iterator().next();
    mCurrentLayer.getController().showImmediately();
  }

  public void applyLastActiveMode()
  {
    toggleMode(mCurrentLayer.getMode(), true);
  }

  @Override
  public void attachCore()
  {
    for (ControllerAndMode each : mLayers)
    {
      each.getController().attachCore();
    }
  }

  @Override
  public void detachCore()
  {
    for (ControllerAndMode each : mLayers)
    {
      each.getController().detachCore();
    }
  }

  private void setMasterController(@NonNull Mode mode)
  {
    for (ControllerAndMode each : mLayers)
    {
      if (each.getMode() == mode)
      {
        mCurrentLayer = each;
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
      mCurrentLayer.getController().show();
    else
      mCurrentLayer.getController().showImmediately();
  }

  @NonNull
  private ControllerAndMode getCurrentLayer()
  {
    for (ControllerAndMode each : mLayers)
    {
      if (each.getMode().isEnabled(mActivity))
        return each;
    }

    return mLayers.iterator().next();
  }

  @Override
  public void turnOn()
  {
    mCurrentLayer.getController().turnOn();
    mCurrentLayer.getMode().setEnabled(mActivity, true);
  }

  @Override
  public void turnOff()
  {
    mCurrentLayer.getController().turnOff();
    mCurrentLayer.getMode().setEnabled(mActivity, false);
  }

  @Override
  public void show()
  {
    mCurrentLayer.getController().show();
  }

  @Override
  public void showImmediately()
  {
    mCurrentLayer.getController().showImmediately();
  }

  @Override
  public void hide()
  {
    mCurrentLayer.getController().hide();
  }

  @Override
  public void hideImmediately()
  {
    mCurrentLayer.getController().hideImmediately();
  }

  @Override
  public void adjust(int offsetX, int offsetY)
  {
    for(ControllerAndMode controllerAndMode: mLayers)
      controllerAndMode.getController().adjust(offsetX, offsetY);
  }

  private void showDialog()
  {
    ToggleMapLayerDialog.show(mActivity);
  }

  @NonNull
  private ControllerAndMode findModeMapLayerController(@NonNull Mode mode)
  {
    for (ControllerAndMode each : mLayers)
    {
      if (each.getMode() == mode)
        return each;
    }

    throw new IllegalArgumentException("Mode not found : " + mode);
  }

  public void turnOnView(@NonNull Mode mode)
  {
    setMasterController(mode);
    mCurrentLayer.getController().showImmediately();
    mCurrentLayer.getController().turnOn();
  }

  public void turnOffCurrentView()
  {
    mCurrentLayer.getController().turnOff();
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
      if (mCurrentLayer.getMode().isEnabled(mActivity))
      {
        Mode mode = getCurrentLayer().getMode();
        Statistics.INSTANCE.trackMapLayerClick(mode, Statistics.ParamValue.MAP, false);
        turnOff();
        toggleMode(mode);
      }
      else
      {
        InputUtils.hideKeyboard(mActivity.getWindow().getDecorView());
        showDialog();
      }
    }
  }
}
