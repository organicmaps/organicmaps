package com.mapswithme.maps.routing;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
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

    Button start = (Button) res.findViewById(R.id.start);
    RoutingController.get().setStartButton(start);
    start.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        RoutingController.get().start();
      }
    });

    return res;
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    mPlanController.disableToggle();
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    RoutingController.get().setStartButton(null);
  }

  public void updatePoints()
  {
    mPlanController.updatePoints();
  }

  public void updateBuildProgress(int progress, int router)
  {
    mPlanController.updateBuildProgress(progress, router);
  }

  @Override
  public boolean onBackPressed()
  {
    return RoutingController.get().cancelPlanning();
  }
}
