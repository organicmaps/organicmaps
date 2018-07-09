package com.mapswithme.maps.routing;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.taxi.TaxiInfo;
import com.mapswithme.maps.taxi.TaxiManager;

public class RoutingPlanFragment extends BaseMwmFragment
                              implements OnBackPressListener
{

  private RoutingPlanController mPlanController;

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View res = inflater.inflate(R.layout.fragment_routing, container, false);
    RoutingBottomMenuListener listener = null;
    if (getActivity() instanceof RoutingBottomMenuListener)
      listener = (RoutingBottomMenuListener) getActivity();
    mPlanController = new RoutingPlanController(res, getActivity(), listener);
    return res;
  }

  public void updateBuildProgress(int progress, @Framework.RouterType int router)
  {
    mPlanController.updateBuildProgress(progress, router);
  }

  public void showTaxiInfo(@NonNull TaxiInfo info)
  {
    mPlanController.showTaxiInfo(info);
  }

  public void showTaxiError(@NonNull TaxiManager.ErrorCode code)
  {
    mPlanController.showTaxiError(code);
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

  public void showAddStartFrame()
  {
    mPlanController.showAddStartFrame();
  }

  public void showAddFinishFrame()
  {
    mPlanController.showAddFinishFrame();
  }

  public void hideActionFrame()
  {
    mPlanController.hideActionFrame();
  }
}
