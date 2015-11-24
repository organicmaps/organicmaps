package com.mapswithme.maps.base;

import android.app.Activity;
import android.os.Bundle;
import android.support.annotation.LayoutRes;
import android.support.v4.app.Fragment;
import android.support.v4.app.NavUtils;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.Toolbar;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import com.mapswithme.maps.R;
import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.util.UiUtils;

public abstract class BaseMwmRecyclerFragment extends Fragment
{
  private Toolbar mToolbar;
  protected RecyclerView mRecycler;

  protected abstract RecyclerView.Adapter createAdapter();

  protected @LayoutRes int getLayoutRes()
  {
    return R.layout.fragment_recycler;
  }

  protected RecyclerView.Adapter getAdapter()
  {
    return mRecycler.getAdapter();
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(getLayoutRes(), container, false);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mToolbar = (Toolbar) view.findViewById(R.id.toolbar);
    if (mToolbar != null)
    {
      UiUtils.showHomeUpButton(mToolbar);
      mToolbar.setNavigationOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          navigateUpToParent();
        }
      });
    }

    mRecycler = (RecyclerView) view.findViewById(R.id.recycler);
    if (mRecycler == null)
      throw new IllegalStateException("RecyclerView not found in layout");

    LinearLayoutManager manager = new LinearLayoutManager(view.getContext());
    mRecycler.setLayoutManager(manager);
    mRecycler.setAdapter(createAdapter());
  }

  public Toolbar getToolbar()
  {
    return mToolbar;
  }

  public RecyclerView getRecyclerView()
  {
    return mRecycler;
  }

  @Override
  public void onResume()
  {
    super.onResume();
    org.alohalytics.Statistics.logEvent("$onResume", this.getClass().getSimpleName()
        + ":" + UiUtils.deviceOrientationAsString(getActivity()));
  }

  @Override
  public void onPause()
  {
    super.onPause();
    org.alohalytics.Statistics.logEvent("$onPause", this.getClass().getSimpleName());
  }

  public void navigateUpToParent()
  {
    final Activity activity = getActivity();
    if (activity instanceof CustomNavigateUpListener)
      ((CustomNavigateUpListener) activity).customOnNavigateUp();
    else
      NavUtils.navigateUpFromSameTask(activity);
  }
}
