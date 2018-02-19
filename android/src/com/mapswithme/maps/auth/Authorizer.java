package com.mapswithme.maps.auth;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.text.TextUtils;

import com.mapswithme.maps.Framework;
import com.mapswithme.util.ConnectionState;

/**
 * An authorizer is responsible for an authorization for the Mapsme server,
 * which is known as "Passport". This process is long and consists two big parts:<br>
 *<br>
 * 1. A user authentication via social networks which results in obtaining
 * the <b>social auth token</b>.<br>
 * 2. A user authorization for the Mapsme Server using the obtained social token on the first step.<br>
 *<br>
 *
 * All callbacks of this authorizer may be listened through {@link Callback} interface. Also there is
 * a method indicating whether the authorization (see step 2) in a progress
 * or not - {@link #isAuthorizationInProgress()}.<br>
 *<br>
 *
 * <b>IMPORTANT NOTE</b>: responsibility of memory leaking (context leaking) is completely
 * on the client of this class. In common case, the careful attaching and detaching to/from instance
 * of this class in activity's/fragment's lifecycle methods, such as onResume()/onPause
 * or onStart()/onStop(), should be enough to avoid memory leaks.
 */
public class Authorizer implements AuthorizationListener
{
  @NonNull
  private final Fragment mFragment;
  @Nullable
  private Callback mCallback;
  private boolean mIsAuthorizationInProgress;

  public Authorizer(@NonNull Fragment fragment)
  {
    mFragment = fragment;
  }

  public void attach(@NonNull Callback callback)
  {
    mCallback = callback;
  }

  public void detach()
  {
    mCallback = null;
  }

  public final void authorize()
  {
    if (isAuthorized() || !ConnectionState.isConnected())
    {
      if (mCallback != null)
        mCallback.onAuthorizationFinish(true);
      return;
    }

    String name = SocialAuthDialogFragment.class.getName();
    DialogFragment fragment = (DialogFragment) Fragment.instantiate(mFragment.getContext(), name);
    fragment.setTargetFragment(mFragment, Constants.REQ_CODE_GET_SOCIAL_TOKEN);
    fragment.show(mFragment.getActivity().getSupportFragmentManager(), name);
  }

  public final void onActivityResult(int requestCode, int resultCode, @Nullable Intent data)
  {

    if (requestCode != Constants.REQ_CODE_GET_SOCIAL_TOKEN)
      return;

    if (data == null)
      return;

    if (resultCode == Activity.RESULT_CANCELED)
    {
      if (mCallback == null)
        return;

      @Framework.AuthTokenType
      int type = data.getIntExtra(Constants.EXTRA_TOKEN_TYPE, -1);
      boolean isCancel = data.getBooleanExtra(Constants.EXTRA_IS_CANCEL, false);
      if (isCancel)
      {
        mCallback.onSocialAuthenticationCancel(type);
        return;
      }

      mCallback.onSocialAuthenticationError(type, data.getStringExtra(Constants.EXTRA_AUTH_ERROR));
      return;
    }

    if (resultCode != Activity.RESULT_OK)
      return;

    String socialToken = data.getStringExtra(Constants.EXTRA_SOCIAL_TOKEN);
    if (!TextUtils.isEmpty(socialToken))
    {
      @Framework.AuthTokenType
      int type = data.getIntExtra(Constants.EXTRA_TOKEN_TYPE, -1);
      mIsAuthorizationInProgress = true;
      if (mCallback != null)
        mCallback.onAuthorizationStart();
      Framework.nativeAuthenticateUser(socialToken, type, this);
    }
  }

  @Override
  public void onAuthorized(boolean success)
  {
    mIsAuthorizationInProgress = false;
    if (mCallback != null)
      mCallback.onAuthorizationFinish(success);
  }

  public boolean isAuthorizationInProgress()
  {
    return mIsAuthorizationInProgress;
  }

  public boolean isAuthorized()
  {
    return Framework.nativeIsUserAuthenticated();
  }

  public interface Callback
  {
    void onAuthorizationFinish(boolean success);
    void onAuthorizationStart();
    void onSocialAuthenticationCancel(@Framework.AuthTokenType int type);
    void onSocialAuthenticationError(@Framework.AuthTokenType int type, @Nullable String error);
  }
}
