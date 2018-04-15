package com.mapswithme.maps.auth;

import android.support.annotation.Nullable;
import android.text.TextUtils;

import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;

class GoogleTokenHandler implements TokenHandler
{
  @Override
  public boolean checkToken()
  {
    GoogleSignInAccount googleAccount = GoogleSignIn.getLastSignedInAccount(MwmApplication.get());
    return googleAccount != null && !TextUtils.isEmpty(googleAccount.getIdToken());
  }

  @Nullable
  @Override
  public String getToken()
  {
    GoogleSignInAccount googleAccount = GoogleSignIn.getLastSignedInAccount(MwmApplication.get());
    return googleAccount != null ? googleAccount.getIdToken() : null;
  }

  @Override
  public int getType()
  {
    return Framework.SOCIAL_TOKEN_GOOGLE;
  }
}
