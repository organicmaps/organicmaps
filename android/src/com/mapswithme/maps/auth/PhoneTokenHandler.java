package com.mapswithme.maps.auth;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.text.TextUtils;

import com.mapswithme.maps.Framework;

public class PhoneTokenHandler implements TokenHandler
{
  @Nullable
  private String mToken;

  @Override
  public boolean checkToken(int requestCode, @NonNull Intent data)
  {
    if (requestCode != Constants.REQ_CODE_PHONE_AUTH_RESULT)
      return false;

    mToken = data.getStringExtra(Constants.EXTRA_PHONE_AUTH_TOKEN);
    return !TextUtils.isEmpty(mToken);
  }

  @Nullable
  @Override
  public String getToken()
  {
    return mToken;
  }

  @Override
  public int getType()
  {
    return Framework.SOCIAL_TOKEN_PHONE;
  }
}
