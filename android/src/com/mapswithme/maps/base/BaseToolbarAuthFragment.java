package com.mapswithme.maps.base;

import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.auth.Authorizer;
import com.mapswithme.maps.auth.TargetFragmentCallback;

/**
 * A base toolbar fragment which is responsible for the <b>authorization flow</b>,
 * starting from the getting an auth token from a social network and passing it to the core
 * to get user authorized for the MapsMe server (Passport).
 */
public abstract class BaseToolbarAuthFragment extends BaseMwmToolbarFragment
    implements Authorizer.Callback, TargetFragmentCallback
{
  @NonNull
  private final Authorizer mAuthorizer = new Authorizer(this);

  protected final void authorize(@NonNull Bundle bundle)
  {
    mAuthorizer.authorize(bundle);
  }

  @Override
  @CallSuper
  public void onStart()
  {
    super.onStart();
    mAuthorizer.attach(this);
  }

  @Override
  @CallSuper
  public void onStop()
  {
    super.onStop();
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
