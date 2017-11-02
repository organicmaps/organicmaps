package com.mapswithme.maps.auth;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.text.TextUtils;
import android.view.View;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.util.ConnectionState;

/**
 * A base toolbar fragment which is responsible for the <b>authorization flow</b>,
 * starting from the getting an auth token from a social network and passing it to the core
 * to get user authorized for the MapsMe server (Passport).
 */
public abstract class BaseMwmAuthorizationFragment extends BaseMwmToolbarFragment
{
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    View submitButton = mToolbarController.findViewById(R.id.submit);
    if (submitButton == null)
      throw new AssertionError("Descendant of BaseMwmAuthorizationFragment must have authorize " +
                               "button in toolbar!");

    submitButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        onSubmitButtonClick();
        authorize();
      }
    });
  }

  private void authorize()
  {
    if (Framework.nativeIsUserAuthenticated() || !ConnectionState.isConnected())
    {
      finishActivity();
      return;
    }

    String name = SocialAuthDialogFragment.class.getName();
    DialogFragment fragment = (DialogFragment) Fragment.instantiate(getContext(), name);
    fragment.setTargetFragment(this, Constants.REQ_CODE_GET_SOCIAL_TOKEN);
    fragment.show(getActivity().getSupportFragmentManager(), name);
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);

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

    finishActivity();
  }

  private void finishActivity()
  {
    if (isAdded())
      getActivity().finish();
  }

  @MainThread
  protected abstract void onSubmitButtonClick();
}
