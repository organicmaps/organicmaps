package com.mapswithme.maps.auth;

import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.statistics.Statistics;

public class PassportAuthDialogFragment extends BaseMwmDialogFragment
    implements TargetFragmentCallback
{
  @NonNull
  private final Authorizer mAuthorizer = new Authorizer(this);

  @NonNull
  private final AuthCallback mAuthCallback = new AuthCallback();

  @Nullable
  private Bundle mSavedInstanceState;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mSavedInstanceState = savedInstanceState;
  }

  @Override
  @CallSuper
  public void onStart()
  {
    super.onStart();
    mAuthorizer.attach(mAuthCallback);
    if (mSavedInstanceState == null)
      mAuthorizer.authorize(getArgumentsOrThrow());
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
    dismiss();
  }

  @Override
  public boolean isTargetAdded()
  {
    return isAdded();
  }

  private class AuthCallback implements Authorizer.Callback
  {
    @Override
    public void onAuthorizationFinish(boolean success)
    {
      dismiss();
      if (success)
      {
        Notifier notifier = Notifier.from(getActivity().getApplication());
        notifier.cancelNotification(Notifier.ID_IS_NOT_AUTHENTICATED);
      }
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
