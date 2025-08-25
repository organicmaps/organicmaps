package app.organicmaps.sdk.sync.googledrive;

import app.organicmaps.sdk.sync.SyncManager;
import app.organicmaps.sdk.sync.SyncOpException;
public class AuthHeader
{
  private final String mRefreshToken;
  private String mAuthHeader;
  private long mExpiresAt = System.currentTimeMillis();

  AuthHeader(String refreshToken)
  {
    mRefreshToken = refreshToken;
  }

  String get() throws SyncOpException
  {
    if (System.currentTimeMillis() > mExpiresAt)
    {
      OAuthHelper.TokenResponse response =
          OAuthHelper.generateAccessToken(SyncManager.INSTANCE.getOkHttpClient(), mRefreshToken);
      mAuthHeader = response.authHeader();
      mExpiresAt = System.currentTimeMillis() + (response.expiresIn() * 1000L) - 120_000L;
    }
    return mAuthHeader;
  }
}
