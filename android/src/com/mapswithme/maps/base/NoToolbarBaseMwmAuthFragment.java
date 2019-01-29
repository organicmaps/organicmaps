package com.mapswithme.maps.base;

import android.content.Context;
import android.content.Intent;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.auth.Authorizer;
import com.mapswithme.maps.auth.TargetFragmentCallback;

public abstract class NoToolbarBaseMwmAuthFragment extends BaseAsyncOperationFragment
    implements Authorizer.Callback, TargetFragmentCallback
{
  @NonNull
  private final Authorizer mAuthorizer = new Authorizer(this);

  protected void authorize()
  {
    mAuthorizer.authorize();
  }

  @Override
  @CallSuper
  public void onAttach(Context context)
  {
    super.onAttach(context);
    mAuthorizer.attach(this);
  }

  @Override
  @CallSuper
  public void onDestroyView()
  {
    super.onDestroyView();
    mAuthorizer.detach();
  }

  @Override
  public void onTargetFragmentResult(int resultCode, @Nullable Intent data)
  {
    mAuthorizer.onTargetFragmentResult(resultCode, data);
  }

  @Override
  public boolean isTargetAdded()
  {
    return isAdded();
  }

  protected boolean isAuthorized()
  {
    return mAuthorizer.isAuthorized();
  }
}
