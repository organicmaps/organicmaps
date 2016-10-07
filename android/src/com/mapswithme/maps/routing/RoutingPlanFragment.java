package com.mapswithme.maps.routing;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.base.OnBackPressListener;

public class RoutingPlanFragment extends BaseMwmFragment
                              implements OnBackPressListener
{

  private RoutingPlanController mPlanController;

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View res = inflater.inflate(R.layout.fragment_routing, container, false);

    mPlanController = new RoutingPlanController(res, getActivity());
    updatePoints();

    Bundle activityState = getMwmActivity().getSavedInstanceState();
    if (activityState != null)
      restoreAltitudeChartState(activityState);

    return res;
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    mPlanController.disableToggle();
  }

  public void updatePoints()
  {
    mPlanController.updatePoints();
  }

  public void updateBuildProgress(int progress, @Framework.RouterType int router)
  {
    mPlanController.updateBuildProgress(progress, router);
  }

  @Override
  public boolean onBackPressed()
  {
    return RoutingController.get().cancelPlanning();
  }

  public void showRouteAltitudeChart()
  {
    mPlanController.showRouteAltitudeChart();
  }

  public void setStartButton()
  {
    mPlanController.setStartButton();
  }

  public void restoreAltitudeChartState(@NonNull Bundle state)
  {
    mPlanController.restoreAltitudeChartState(state);
  }

  public void saveAltitudeChartState(@NonNull Bundle outState)
  {
    mPlanController.saveAltitudeChartState(outState);
  }
}
