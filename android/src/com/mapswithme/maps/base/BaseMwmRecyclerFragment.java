package com.mapswithme.maps.base;

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

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.PlaceholderView;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

public abstract class BaseMwmRecyclerFragment extends Fragment
{
  private Toolbar mToolbar;
  @Nullable
  private RecyclerView mRecycler;
  private PlaceholderView mPlaceholder;

  @Nullable
  private View mView;
  @Nullable
  private Bundle mSavedInstanceState;

  protected abstract RecyclerView.Adapter createAdapter();

  protected @LayoutRes int getLayoutRes()
  {
    return R.layout.fragment_recycler;
  }

  @Nullable
  protected RecyclerView.Adapter getAdapter()
  {
    return mRecycler != null ? mRecycler.getAdapter() : null;
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

    mView = view;
    mSavedInstanceState = savedInstanceState;
    if (MwmApplication.get().isPlatformInitialized() && Utils.isWriteExternalGranted(getActivity()))
      safeOnViewCreated(view, savedInstanceState);
  }

  @CallSuper
  protected void safeOnViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    mToolbar = (Toolbar) view.findViewById(R.id.toolbar);
    if (mToolbar != null)
    {
      UiUtils.showHomeUpButton(mToolbar);
      mToolbar.setNavigationOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          Utils.navigateToParent(getActivity());
        }
      });
    }

    mRecycler = (RecyclerView) view.findViewById(R.id.recycler);
    if (mRecycler == null)
      throw new IllegalStateException("RecyclerView not found in layout");

    LinearLayoutManager manager = new LinearLayoutManager(view.getContext());
    manager.setSmoothScrollbarEnabled(true);
    mRecycler.setLayoutManager(manager);
    mRecycler.setAdapter(createAdapter());

    mPlaceholder = (PlaceholderView) view.findViewById(R.id.placeholder);
    setupPlaceholder(mPlaceholder);
  }

  @CallSuper
  @Override
  public void onDestroyView()
  {
    mView = null;
    mSavedInstanceState = null;
    super.onDestroyView();
  }

  @Override
  public void onActivityCreated(@Nullable Bundle savedInstanceState)
  {
    super.onActivityCreated(savedInstanceState);
    if (MwmApplication.get().isPlatformInitialized() && Utils.isWriteExternalGranted(getActivity()))
      safeOnActivityCreated(savedInstanceState);
  }

  protected void safeOnActivityCreated(@Nullable Bundle savedInstanceState)
  {
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults)
  {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    if (grantResults.length == 0)
      return;

    boolean isWriteGranted = false;
    for (int i = 0; i < permissions.length; ++i)
    {
      int result = grantResults[i];
      String permission = permissions[i];
      if (permission.equals(WRITE_EXTERNAL_STORAGE) && result == PERMISSION_GRANTED)
        isWriteGranted = true;
    }

    if (isWriteGranted && mView != null)
    {
      safeOnViewCreated(mView, mSavedInstanceState);
      safeOnActivityCreated(mSavedInstanceState);
    }
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

  protected void setupPlaceholder(@NonNull PlaceholderView placeholder) {}

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
