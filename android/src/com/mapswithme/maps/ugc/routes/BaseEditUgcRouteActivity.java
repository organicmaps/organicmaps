package com.mapswithme.maps.ugc.routes;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.DataObservable;
import com.mapswithme.maps.base.ObservableHost;

public abstract class BaseEditUgcRouteActivity extends BaseMwmFragmentActivity implements ObservableHost<DataObservable>
{
  @SuppressWarnings("NullableProblems")
  @NonNull
  private DataObservable mObservable;

  @Override
  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
    super.safeOnCreate(savedInstanceState);
    mObservable = new DataObservable();
  }

  @NonNull
  @Override
  public DataObservable getObservable()
  {
    return mObservable;
  }
}
