package app.organicmaps.sdk.editor;

import android.content.SharedPreferences;
import android.graphics.Bitmap;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.Size;
import androidx.annotation.WorkerThread;
import androidx.fragment.app.FragmentManager;
import app.organicmaps.sdk.util.NetworkPolicy;

public final class OsmOAuth
{
  private OsmOAuth() {}

  public enum AuthType
  {
    OSM("OSM"),
    GOOGLE("Google");

    public final String name;

    AuthType(String name)
    {
      this.name = name;
    }
  }

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private static SharedPreferences mPrefs;

  private static final String PREF_OSM_TOKEN = "OsmToken"; // Unused after migration from OAuth1 to OAuth2
  private static final String PREF_OSM_SECRET = "OsmSecret"; // Unused after migration from OAuth1 to OAuth2
  private static final String PREF_OSM_USERNAME = "OsmUsername";
  private static final String PREF_OSM_CHANGESETS_COUNT = "OsmChangesetsCount";
  private static final String PREF_OSM_OAUTH2_TOKEN = "OsmOAuth2Token";

  public static final String URL_PARAM_VERIFIER = "oauth_verifier";

  public static void init(@NonNull SharedPreferences prefs)
  {
    mPrefs = prefs;
  }

  public static boolean isAuthorized()
  {
    return mPrefs.contains(PREF_OSM_OAUTH2_TOKEN);
  }

  public static boolean containsOAuth1Credentials()
  {
    return mPrefs.contains(PREF_OSM_TOKEN) && mPrefs.contains(PREF_OSM_SECRET);
  }

  public static void clearOAuth1Credentials()
  {
    mPrefs.edit().remove(PREF_OSM_TOKEN).remove(PREF_OSM_SECRET).apply();
  }

  public static String getAuthToken()
  {
    return mPrefs.getString(PREF_OSM_OAUTH2_TOKEN, "");
  }

  public static String getUsername()
  {
    return mPrefs.getString(PREF_OSM_USERNAME, "");
  }

  public static Bitmap getProfilePicture()
  {
    // TODO(HB): load and store image in cache here
    return null;
  }

  public static void setAuthorization(String oauthToken, String username)
  {
    mPrefs.edit().putString(PREF_OSM_OAUTH2_TOKEN, oauthToken).putString(PREF_OSM_USERNAME, username).apply();
  }

  public static void clearAuthorization()
  {
    mPrefs.edit()
        .remove(PREF_OSM_TOKEN)
        .remove(PREF_OSM_SECRET)
        .remove(PREF_OSM_USERNAME)
        .remove(PREF_OSM_OAUTH2_TOKEN)
        .apply();
  }

  @NonNull
  public static String getHistoryUrl()
  {
    return nativeGetHistoryUrl(getUsername());
  }

  @NonNull
  public static String getNotesUrl()
  {
    return nativeGetNotesUrl(getUsername());
  }

  /*
   Returns 5 strings: ServerURL, ClientId, ClientSecret, Scope, RedirectUri
   */
  @NonNull
  public static native String nativeGetOAuth2Url();

  /**
   * @return string with OAuth2 token
   */
  @WorkerThread
  @Size(2)
  @Nullable
  public static native String nativeAuthWithPassword(String login, String password);

  /**
   * @return string with OAuth2 token
   */
  @WorkerThread
  @Nullable
  public static native String nativeAuthWithOAuth2Code(String oauth2code);

  @WorkerThread
  @Nullable
  public static native String nativeGetOsmUsername(String oauthToken);

  @WorkerThread
  @Nullable
  public static native String nativeGetOsmProfilePictureUrl(String oauthToken);

  @WorkerThread
  @NonNull
  public static native String nativeGetHistoryUrl(String user);

  @WorkerThread
  @NonNull
  public static native String nativeGetNotesUrl(String user);

  /**
   * @return < 0 if failed to get changesets count.
   */
  @WorkerThread
  private static native int nativeGetOsmChangesetsCount(String oauthToken);

  @WorkerThread
  public static int getOsmChangesetsCount(@NonNull FragmentManager fm)
  {
    final int[] editsCount = {-1};
    NetworkPolicy.checkNetworkPolicy(fm, policy -> {
      if (!policy.canUseNetwork())
        return;

      final String token = getAuthToken();
      editsCount[0] = OsmOAuth.nativeGetOsmChangesetsCount(token);
    });
    if (editsCount[0] < 0)
      return mPrefs.getInt(PREF_OSM_CHANGESETS_COUNT, 0);

    mPrefs.edit().putInt(PREF_OSM_CHANGESETS_COUNT, editsCount[0]).apply();
    return editsCount[0];
  }
}
