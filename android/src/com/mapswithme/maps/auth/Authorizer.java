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

public class Authorizer
{
  @NonNull
  private final Fragment mFragment;
  @Nullable
  private final Callback mCallback;

  public Authorizer(@NonNull Fragment fragment, @NonNull Callback callback)
  {
    mFragment = fragment;
    mCallback = callback;
  }

  public final void authorize()
  {
    if (Framework.nativeIsUserAuthenticated() || !ConnectionState.isConnected())
    {
      if (mCallback != null)
        mCallback.onAuthorizationFinish();
      return;
    }

    String name = SocialAuthDialogFragment.class.getName();
    DialogFragment fragment = (DialogFragment) Fragment.instantiate(mFragment.getContext(), name);
    fragment.setTargetFragment(mFragment, Constants.REQ_CODE_GET_SOCIAL_TOKEN);
    fragment.show(mFragment.getActivity().getSupportFragmentManager(), name);
  }

  public final void onActivityResult(int requestCode, int resultCode, Intent data)
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
      Framework.nativeAuthenticateUser(socialToken, type);
    }

    if (mCallback != null)
      mCallback.onAuthorizationFinish();
  }

  public interface Callback
  {
    void onAuthorizationFinish();
  }
}
