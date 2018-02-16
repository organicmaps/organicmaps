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

public class Authorizer implements AuthorizationListener
{
  @NonNull
  private final Fragment mFragment;
  @Nullable
  private Callback mCallback;
  private boolean mInProgress;

  public Authorizer(@NonNull Fragment fragment)
  {
    this(fragment, null);
  }

  Authorizer(@NonNull Fragment fragment, @Nullable Callback callback)
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
    if (requestCode != Constants.REQ_CODE_GET_SOCIAL_TOKEN
        || resultCode != Activity.RESULT_OK || data == null)
    {
      return;
    }

    String socialToken = data.getStringExtra(Constants.EXTRA_SOCIAL_TOKEN);
    if (!TextUtils.isEmpty(socialToken))
    {
      @Framework.SocialTokenType
      int type = data.getIntExtra(Constants.EXTRA_TOKEN_TYPE, -1);
      mInProgress = true;
      if (mCallback != null)
        mCallback.onAuthorizationStart();
      Framework.nativeAuthenticateUser(socialToken, type, this);
    }
  }

  @Override
  public void onAuthorized(boolean success)
  {
    mInProgress = false;
    if (mCallback != null)
      mCallback.onAuthorizationFinish(success);
  }

  public boolean isInProgress()
  {
    return mInProgress;
  }

  public boolean isAuthorized()
  {
    return Framework.nativeIsUserAuthenticated();
  }

  public interface Callback
  {
    void onAuthorizationFinish(boolean success);
    void onAuthorizationStart();
  }
}
