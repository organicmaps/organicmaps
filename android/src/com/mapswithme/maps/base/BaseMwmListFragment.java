package com.mapswithme.maps.base;

import android.content.Context;
import android.os.Bundle;
import android.view.View;

import androidx.appcompat.widget.Toolbar;
import androidx.fragment.app.ListFragment;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

@Deprecated
public abstract class BaseMwmListFragment extends ListFragment
{
  private Toolbar mToolbar;

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);
    Utils.detachFragmentIfCoreNotInitialized(context, this);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mToolbar = view.findViewById(R.id.toolbar);
    if (mToolbar != null)
    {
      UiUtils.setupNavigationIcon(mToolbar, v -> Utils.navigateToParent(getActivity()));
    }
  }

  public Toolbar getToolbar()
  {
    return mToolbar;
  }

  @Override
  public void onResume()
  {
    super.onResume();
    org.alohalytics.Statistics.logEvent("$onResume", getClass().getSimpleName() + ":" +
                                                     UiUtils.deviceOrientationAsString(requireActivity()));
  }

  @Override
  public void onPause()
  {
    super.onPause();
    org.alohalytics.Statistics.logEvent("$onPause", getClass().getSimpleName());
  }
}
