package com.mapswithme.maps.auth;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.text.TextUtils;

import com.facebook.AccessToken;
import com.mapswithme.maps.Framework;

class FacebookTokenHandler implements TokenHandler
{
  @Override
  public boolean checkToken(int requestCode, @NonNull Intent data)
  {
    AccessToken facebookToken = AccessToken.getCurrentAccessToken();
    return facebookToken != null && !TextUtils.isEmpty(facebookToken.getToken());
  }

  @Nullable
  @Override
  public String getToken()
  {
    AccessToken facebookToken = AccessToken.getCurrentAccessToken();
    return facebookToken != null ? facebookToken.getToken() : null;
  }

  @Override
  public int getType()
  {
    return Framework.SOCIAL_TOKEN_FACEBOOK;
  }
}
