package com.mapswithme.maps.auth;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.statistics.Statistics;

public class PassportAuthDialogFragment extends BaseMwmDialogFragment
    implements Authorizer.SocialAuthCallback
{
  @NonNull
  private final Authorizer mAuthorizer = new Authorizer(this);
  @NonNull
  private final AuthCallback mAuthCallback = new AuthCallback();

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    if (savedInstanceState == null)
      mAuthorizer.authorize();

    return null;
  }

  @Override
  @CallSuper
  public void onStart()
  {
    super.onStart();
    mAuthorizer.attach(mAuthCallback);
  }

  @Override
  @CallSuper
  public void onStop()
  {
    super.onStop();
    mAuthorizer.detach();
  }

  @Override
  public void onSocialTokenResult(int resultCode, @Nullable Intent data)
  {
    mAuthorizer.onSocialTokenResult(resultCode, data);
    dismiss();
  }

  private class AuthCallback implements Authorizer.Callback
  {
    @Override
    public void onAuthorizationFinish(boolean success)
    {
      dismiss();

      if (success)
        Notifier.cancelNotification(Notifier.ID_IS_NOT_AUTHENTICATED);
    }

    @Override
    public void onAuthorizationStart()
    {
    }

    @Override
    public void onSocialAuthenticationCancel(@Framework.AuthTokenType int type)
    {
      Statistics.INSTANCE.trackEvent(Statistics.EventName.UGC_AUTH_DECLINED);
    }

    @Override
    public void onSocialAuthenticationError(@Framework.AuthTokenType int type,
                                            @Nullable String error)
    {
      Statistics.INSTANCE.trackUGCAuthFailed(type, error);
    }
  }
}
