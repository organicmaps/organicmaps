package app.organicmaps.location;

import android.content.Context;
import android.content.SharedPreferences;
import android.location.Location;
import android.util.Base64;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.PreferenceManager;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class RiderTrackingManager implements LocationListener
{
  private static final String TAG = RiderTrackingManager.class.getSimpleName();

  private static final String PREF_SERVER_URL = "rider_server_url";
  private static final String PREF_SERVER_USER = "rider_server_user";
  private static final String PREF_SERVER_PASS = "rider_server_pass";
  private static final String PREF_MY_CODE = "rider_my_code";
  private static final String PREF_MY_NAME = "rider_my_name";
  private static final String PREF_SHARING_ENABLED = "rider_sharing_enabled";
  private static final String PREF_SHOW_MYSELF = "rider_show_myself";
  private static final String PREF_FRIEND_CODES = "rider_friend_codes";

  private static final String DEFAULT_SERVER_URL = "http://10.0.2.2:8080";
  private static final long POLL_INTERVAL_SECONDS = 5;

  private static RiderTrackingManager sInstance;

  private final Context mContext;
  private final SharedPreferences mPrefs;
  private final ScheduledExecutorService mExecutor = Executors.newSingleThreadScheduledExecutor();

  private final Map<String, FriendRiderLocation> mFriendLocations = new HashMap<>();
  private final List<RiderLocationUpdateListener> mListeners = new ArrayList<>();

  public static class FriendRiderLocation
  {
    public final String code;
    public final String name;
    public final double lat;
    public final double lon;
    public final double speed;
    public final double bearing;
    public final long updatedAt;

    public FriendRiderLocation(String code, String name, double lat, double lon, double speed, double bearing,
                               long updatedAt)
    {
      this.code = code;
      this.name = name;
      this.lat = lat;
      this.lon = lon;
      this.speed = speed;
      this.bearing = bearing;
      this.updatedAt = updatedAt;
    }

    /** Label to display on the map: name if set, otherwise the code. */
    @NonNull
    public String displayLabel()
    {
      return (name == null || name.isEmpty()) ? code : name;
    }
  }

  public interface RiderLocationUpdateListener
  {
    void onFriendLocationsUpdated(@NonNull Map<String, FriendRiderLocation> locations);
  }

  public static synchronized RiderTrackingManager get(@NonNull Context context)
  {
    if (sInstance == null)
      sInstance = new RiderTrackingManager(context.getApplicationContext());
    return sInstance;
  }

  private RiderTrackingManager(@NonNull Context context)
  {
    mContext = context;
    mPrefs = PreferenceManager.getDefaultSharedPreferences(mContext);

    // Initialize My Rider Code if not generated yet
    if (getMyRiderCode().isEmpty())
    {
      setMyRiderCode(generateRandomCode());
    }

    // Attach location listener
    MwmApplication.from(mContext).getLocationHelper().addListener(this);

    // Start periodic background polling for friend locations
    mExecutor.scheduleWithFixedDelay(this::pollFriendLocations, 2, POLL_INTERVAL_SECONDS, TimeUnit.SECONDS);
  }

  @NonNull
  public String getServerUrl()
  {
    return mPrefs.getString(PREF_SERVER_URL, DEFAULT_SERVER_URL);
  }

  public void setServerUrl(@NonNull String url)
  {
    mPrefs.edit().putString(PREF_SERVER_URL, url.trim()).apply();
  }

  @NonNull
  public String getServerUsername()
  {
    return mPrefs.getString(PREF_SERVER_USER, "");
  }

  public void setServerUsername(@NonNull String user)
  {
    mPrefs.edit().putString(PREF_SERVER_USER, user.trim()).apply();
  }

  @NonNull
  public String getServerPassword()
  {
    return mPrefs.getString(PREF_SERVER_PASS, "");
  }

  public void setServerPassword(@NonNull String pass)
  {
    // Passwords aren't trimmed — leading/trailing whitespace can be intentional.
    mPrefs.edit().putString(PREF_SERVER_PASS, pass).apply();
  }

  /** Returns "Basic base64(user:pass)" or null if creds are not set. */
  @Nullable
  private String buildAuthHeader()
  {
    String user = getServerUsername();
    String pass = getServerPassword();
    if (user.isEmpty() || pass.isEmpty())
      return null;
    String creds = user + ":" + pass;
    return "Basic " + Base64.encodeToString(creds.getBytes(StandardCharsets.UTF_8), Base64.NO_WRAP);
  }

  @NonNull
  public String getMyRiderCode()
  {
    return mPrefs.getString(PREF_MY_CODE, "");
  }

  public void setMyRiderCode(@NonNull String code)
  {
    mPrefs.edit().putString(PREF_MY_CODE, code.toUpperCase().trim()).apply();
  }

  @NonNull
  public String getMyName()
  {
    return mPrefs.getString(PREF_MY_NAME, "");
  }

  public void setMyName(@NonNull String name)
  {
    mPrefs.edit().putString(PREF_MY_NAME, name.trim()).apply();
  }

  public boolean isSharingEnabled()
  {
    return mPrefs.getBoolean(PREF_SHARING_ENABLED, false);
  }

  public void setSharingEnabled(boolean enabled)
  {
    mPrefs.edit().putBoolean(PREF_SHARING_ENABLED, enabled).apply();
  }

  public boolean isShowMyselfEnabled()
  {
    return mPrefs.getBoolean(PREF_SHOW_MYSELF, false);
  }

  public void setShowMyselfEnabled(boolean enabled)
  {
    mPrefs.edit().putBoolean(PREF_SHOW_MYSELF, enabled).apply();
    if (!enabled)
    {
      // Drop own entry from the cached map so the overlay stops drawing it immediately.
      String myCode = getMyRiderCode();
      synchronized (mFriendLocations)
      {
        mFriendLocations.remove(myCode);
      }
    }
  }

  @NonNull
  public Set<String> getFriendCodes()
  {
    return mPrefs.getStringSet(PREF_FRIEND_CODES, Collections.emptySet());
  }

  public void addFriendCode(@NonNull String code)
  {
    String cleanCode = code.toUpperCase().trim();
    if (cleanCode.isEmpty())
      return;
    Set<String> set = new HashSet<>(getFriendCodes());
    set.add(cleanCode);
    mPrefs.edit().putStringSet(PREF_FRIEND_CODES, set).apply();
  }

  public void removeFriendCode(@NonNull String code)
  {
    Set<String> set = new HashSet<>(getFriendCodes());
    set.remove(code.toUpperCase().trim());
    mPrefs.edit().putStringSet(PREF_FRIEND_CODES, set).apply();
    mFriendLocations.remove(code.toUpperCase().trim());
  }

  public void addListener(@NonNull RiderLocationUpdateListener listener)
  {
    if (!mListeners.contains(listener))
      mListeners.add(listener);
  }

  public void removeListener(@NonNull RiderLocationUpdateListener listener)
  {
    mListeners.remove(listener);
  }

  @NonNull
  public Map<String, FriendRiderLocation> getFriendLocations()
  {
    synchronized (mFriendLocations)
    {
      return new HashMap<>(mFriendLocations);
    }
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    if (!isSharingEnabled())
      return;

    mExecutor.execute(() -> publishCurrentLocation(location));
  }

  private void publishCurrentLocation(@NonNull Location location)
  {
    String myCode = getMyRiderCode();
    String serverUrl = getServerUrl();

    if (myCode.isEmpty() || serverUrl.isEmpty())
      return;

    HttpURLConnection conn = null;
    try
    {
      URL url = new URL(serverUrl.endsWith("/") ? serverUrl + "api/location" : serverUrl + "/api/location");
      conn = (HttpURLConnection) url.openConnection();
      conn.setRequestMethod("POST");
      conn.setRequestProperty("Content-Type", "application/json");
      String auth = buildAuthHeader();
      if (auth != null)
        conn.setRequestProperty("Authorization", auth);
      conn.setConnectTimeout(4000);
      conn.setReadTimeout(4000);
      conn.setDoOutput(true);

      JSONObject payload = new JSONObject();
      payload.put("code", myCode);
      payload.put("name", getMyName());
      payload.put("lat", location.getLatitude());
      payload.put("lon", location.getLongitude());
      payload.put("speed", location.hasSpeed() ? location.getSpeed() : 0.0);
      payload.put("bearing", location.hasBearing() ? location.getBearing() : 0.0);

      byte[] bodyBytes = payload.toString().getBytes(StandardCharsets.UTF_8);
      try (OutputStream os = conn.getOutputStream())
      {
        os.write(bodyBytes);
      }

      int responseCode = conn.getResponseCode();
      if (responseCode == 200)
      {
        Logger.d(TAG, "Location published successfully for " + myCode);
      }
      else
      {
        Logger.w(TAG, "Publish location returned status code: " + responseCode);
      }
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to publish location to server", e);
    }
    finally
    {
      if (conn != null)
        conn.disconnect();
    }
  }

  private void pollFriendLocations()
  {
    // Effective set = configured friends plus self when "Show myself" is enabled (for testing).
    Set<String> friendCodes = new HashSet<>(getFriendCodes());
    if (isShowMyselfEnabled())
    {
      String myCode = getMyRiderCode();
      if (!myCode.isEmpty())
        friendCodes.add(myCode);
    }
    String serverUrl = getServerUrl();

    if (friendCodes.isEmpty() || serverUrl.isEmpty())
      return;

    HttpURLConnection conn = null;
    try
    {
      URL url = new URL(serverUrl.endsWith("/") ? serverUrl + "api/locations/batch" : serverUrl + "/api/locations/batch");
      conn = (HttpURLConnection) url.openConnection();
      conn.setRequestMethod("POST");
      conn.setRequestProperty("Content-Type", "application/json");
      String auth = buildAuthHeader();
      if (auth != null)
        conn.setRequestProperty("Authorization", auth);
      conn.setConnectTimeout(4000);
      conn.setReadTimeout(4000);
      conn.setDoOutput(true);

      JSONObject payload = new JSONObject();
      JSONArray codesArray = new JSONArray();
      for (String code : friendCodes)
        codesArray.put(code);
      payload.put("codes", codesArray);

      byte[] bodyBytes = payload.toString().getBytes(StandardCharsets.UTF_8);
      try (OutputStream os = conn.getOutputStream())
      {
        os.write(bodyBytes);
      }

      int responseCode = conn.getResponseCode();
      if (responseCode == 200)
      {
        InputStream is = conn.getInputStream();
        byte[] buffer = new byte[4096];
        StringBuilder sb = new StringBuilder();
        int bytesRead;
        while ((bytesRead = is.read(buffer)) != -1)
        {
          sb.append(new String(buffer, 0, bytesRead, StandardCharsets.UTF_8));
        }

        JSONObject resp = new JSONObject(sb.toString());
        if (resp.has("riders"))
        {
          JSONObject riders = resp.getJSONObject("riders");
          Map<String, FriendRiderLocation> updatedMap = new HashMap<>();

          for (String code : friendCodes)
          {
            if (riders.has(code))
            {
              JSONObject r = riders.getJSONObject(code);
              updatedMap.put(code, new FriendRiderLocation(
                  code,
                  r.optString("name", ""),
                  r.getDouble("lat"),
                  r.getDouble("lon"),
                  r.optDouble("speed", 0.0),
                  r.optDouble("bearing", 0.0),
                  r.optLong("updated_at", System.currentTimeMillis() / 1000)
              ));
            }
          }

          synchronized (mFriendLocations)
          {
            mFriendLocations.clear();
            mFriendLocations.putAll(updatedMap);
          }

          UiThread.run(() -> {
            for (RiderLocationUpdateListener l : mListeners)
              l.onFriendLocationsUpdated(updatedMap);
          });
        }
      }
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to poll friend rider locations", e);
    }
    finally
    {
      if (conn != null)
        conn.disconnect();
    }
  }

  @NonNull
  private static String generateRandomCode()
  {
    String chars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    SecureRandom rnd = new SecureRandom();
    StringBuilder sb = new StringBuilder("RIDER-");
    for (int i = 0; i < 5; i++)
      sb.append(chars.charAt(rnd.nextInt(chars.length())));
    return sb.toString();
  }
}
