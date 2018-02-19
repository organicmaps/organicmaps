package com.mapswithme.maps.auth;

import android.content.Intent;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;

import com.mapswithme.maps.base.BaseMwmToolbarFragment;

/**
 * A base toolbar fragment which is responsible for the <b>authorization flow</b>,
 * starting from the getting an auth token from a social network and passing it to the core
 * to get user authorized for the MapsMe server (Passport).
 */
public abstract class BaseMwmAuthorizationFragment extends BaseMwmToolbarFragment
    implements Authorizer.Callback
{
  @NonNull
  private final Authorizer mAuthorizer = new Authorizer(this);

  protected void authorize()
  {
    mAuthorizer.authorize();
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
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    mAuthorizer.onActivityResult(requestCode, resultCode, data);
  }
}
