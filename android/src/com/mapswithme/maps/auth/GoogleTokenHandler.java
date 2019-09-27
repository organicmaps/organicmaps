package com.mapswithme.maps.auth;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.text.TextUtils;

import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.tasks.Task;
import com.mapswithme.maps.Framework;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

class GoogleTokenHandler implements TokenHandler
{
  @Nullable
  private String mToken;

  @Override
  public boolean checkToken(int requestCode, @NonNull Intent data)
  {
    if (requestCode != Constants.REQ_CODE_GOOGLE_SIGN_IN)
      return false;

    Task<GoogleSignInAccount> task = GoogleSignIn.getSignedInAccountFromIntent(data);
    try
    {
      GoogleSignInAccount account = task.getResult(ApiException.class);
      if (account != null)
        mToken = account.getIdToken();
      return !TextUtils.isEmpty(mToken);
    }
    catch (ApiException e)
    {
      // The ApiException status code indicates the detailed failure reason.
      // Please refer to the GoogleSignInStatusCodes class reference for more information.
      Logger logger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
      logger.w(GoogleTokenHandler.class.getSimpleName(),
               "signInResult:failed code=" + e.getStatusCode());
    }
    return false;
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
    return Framework.SOCIAL_TOKEN_GOOGLE;
  }
}
