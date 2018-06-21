package com.mapswithme.maps.base;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.LayoutRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.Toolbar;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.PlaceholderView;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

public abstract class BaseMwmRecyclerFragment<T extends RecyclerView.Adapter> extends Fragment
{
  private Toolbar mToolbar;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private RecyclerView mRecycler;

  @Nullable
  private PlaceholderView mPlaceholder;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private T mAdapter;

  @NonNull
  protected abstract T createAdapter();

  @LayoutRes
  protected int getLayoutRes()
  {
    return R.layout.fragment_recycler;
  }

  @NonNull
  protected T getAdapter()
  {
    return mAdapter;
  }

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);
    Utils.detachFragmentIfCoreNotInitialized(context, this);
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(getLayoutRes(), container, false);
  }

  @CallSuper
  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mToolbar = view.findViewById(R.id.toolbar);
    if (mToolbar != null)
    {
      UiUtils.showHomeUpButton(mToolbar);
      mToolbar.setNavigationOnClickListener(v -> Utils.navigateToParent(getActivity()));
    }

    mRecycler = view.findViewById(R.id.recycler);
    if (mRecycler == null)
      throw new IllegalStateException("RecyclerView not found in layout");

    LinearLayoutManager manager = new LinearLayoutManager(view.getContext());
    manager.setSmoothScrollbarEnabled(true);
    mRecycler.setLayoutManager(manager);
    mAdapter = createAdapter();
    mRecycler.setAdapter(mAdapter);

    mPlaceholder = view.findViewById(R.id.placeholder);
    setupPlaceholder(mPlaceholder);
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

  protected void setupPlaceholder(@Nullable PlaceholderView placeholder) {}

  public void setupPlaceholder()
  {
    setupPlaceholder(mPlaceholder);
  }

  public void showPlaceholder(boolean show)
  {
    if (mPlaceholder != null)
      UiUtils.showIf(show, mPlaceholder);
  }
}
