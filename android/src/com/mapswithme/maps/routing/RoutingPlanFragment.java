package com.mapswithme.maps.routing;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;

public class RoutingPlanFragment extends BaseMwmFragment
{
  private View mFrame;
  private RoutingPlanController mPlanController;

  public RoutingPlanController getPlanController()
  {
    return mPlanController;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_routing, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    mFrame = view;
  }

  @Override
  public void onActivityCreated(@Nullable Bundle savedInstanceState)
  {
    super.onActivityCreated(savedInstanceState);
    mPlanController = new RoutingPlanController(mFrame, getActivity());
    mPlanController.disableToggle();
    mPlanController.updatePoints();
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();

  }
}
