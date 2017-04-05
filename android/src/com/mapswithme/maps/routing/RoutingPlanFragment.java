package com.mapswithme.maps.routing;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.uber.Uber;
import com.mapswithme.maps.uber.UberInfo;
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

  public void showUberInfo(@NonNull UberInfo info)
  {
    mPlanController.showUberInfo(info);
  }

  public void showUberError(@NonNull Uber.ErrorCode code)
  {
    mPlanController.showUberError(code);
  }

  @Override
  public boolean onBackPressed()
  {
    return RoutingController.get().cancel();
  }

  public void restoreRoutingPanelState(@NonNull Bundle state)
  {
    mPlanController.restoreRoutingPanelState(state);
  }

  public void saveRoutingPanelState(@NonNull Bundle outState)
  {
    mPlanController.saveRoutingPanelState(outState);
  }
}
