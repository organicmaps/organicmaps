package app.organicmaps.bookmarks.cloud;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.concurrent.TimeUnit;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

/**
 * High-performance, lightweight Nextcloud WebDAV client for syncing Organic Maps bookmarks.
 * Complies with RFC 4918 (WebDAV) without adding heavy third-party SDK dependencies.
 */
public class NextcloudWebDavClient
{
  private static final String TAG = NextcloudWebDavClient.class.getSimpleName();
  private static final String DEFAULT_REMOTE_PATH = "OrganicMaps/Bookmarks";

  private final OkHttpClient mHttpClient;
  private final String mServerBaseUrl;
  private final String mAuthHeader;

  public NextcloudWebDavClient(@NonNull String serverUrl, @NonNull String username, @NonNull String passwordOrToken)
  {
    mHttpClient = new OkHttpClient.Builder()
        .connectTimeout(30, TimeUnit.SECONDS)
        .readTimeout(60, TimeUnit.SECONDS)
        .writeTimeout(60, TimeUnit.SECONDS)
        .build();

    String normalizedUrl = serverUrl.trim();
    if (normalizedUrl.endsWith("/"))
      normalizedUrl = normalizedUrl.substring(0, normalizedUrl.length() - 1);

    // Standard Nextcloud WebDAV root: /remote.php/dav/files/<username>/
    if (!normalizedUrl.contains("/remote.php/dav/files/"))
      mServerBaseUrl = normalizedUrl + "/remote.php/dav/files/" + username + "/";
    else
      mServerBaseUrl = normalizedUrl.endsWith("/") ? normalizedUrl : normalizedUrl + "/";

    String credentials = username + ":" + passwordOrToken;
    mAuthHeader = "Basic " + Base64.getEncoder().encodeToString(credentials.getBytes(StandardCharsets.UTF_8));
  }

  /**
   * Ensures remote backup directories exist on Nextcloud instance (via MKCOL).
   */
  public boolean ensureRemoteDirectoryExists()
  {
    String targetUrl = mServerBaseUrl + DEFAULT_REMOTE_PATH + "/";
    Request checkReq = new Request.Builder()
        .url(targetUrl)
        .header("Authorization", mAuthHeader)
        .method("PROPFIND", RequestBody.create(new byte[0], null))
        .header("Depth", "0")
        .build();

    try (Response resp = mHttpClient.newCall(checkReq).execute())
    {
      if (resp.isSuccessful() || resp.code() == 207)
        return true;
    }
    catch (IOException e)
    {
      Logger.w(TAG, "Directory PROPFIND failed, attempting MKCOL: " + e.getMessage());
    }

    // Attempt recursive MKCOL for parent and subfolder
    String[] parts = DEFAULT_REMOTE_PATH.split("/");
    StringBuilder currentPath = new StringBuilder(mServerBaseUrl);
    for (String part : parts)
    {
      currentPath.append(part).append("/");
      Request mkcolReq = new Request.Builder()
          .url(currentPath.toString())
          .header("Authorization", mAuthHeader)
          .method("MKCOL", null)
          .build();
      try (Response resp = mHttpClient.newCall(mkcolReq).execute())
      {
        if (!resp.isSuccessful() && resp.code() != 405) // 405 Method Not Allowed means already exists
        {
          Logger.e(TAG, "MKCOL failed for path " + currentPath + " code: " + resp.code());
          return false;
        }
      }
      catch (IOException e)
      {
        Logger.e(TAG, "MKCOL network error: " + e.getMessage());
        return false;
      }
    }
    return true;
  }

  /**
   * Uploads bookmark KMZ/KML archive to Nextcloud WebDAV storage.
   */
  public boolean uploadBookmarkFile(@NonNull File localFile, @NonNull String remoteFileName)
  {
    if (!ensureRemoteDirectoryExists())
      return false;

    String uploadUrl = mServerBaseUrl + DEFAULT_REMOTE_PATH + "/" + remoteFileName;
    MediaType mediaType = MediaType.parse("application/vnd.google-earth.kmz");
    RequestBody body = RequestBody.create(localFile, mediaType);

    Request req = new Request.Builder()
        .url(uploadUrl)
        .header("Authorization", mAuthHeader)
        .put(body)
        .build();

    try (Response resp = mHttpClient.newCall(req).execute())
    {
      if (resp.isSuccessful() || resp.code() == 201 || resp.code() == 204)
      {
        Logger.i(TAG, "Bookmark backup upload successful: " + remoteFileName);
        return true;
      }
      Logger.e(TAG, "Bookmark upload returned error code: " + resp.code());
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Bookmark upload failed: " + e.getMessage());
    }
    return false;
  }

  /**
   * Downloads latest bookmark archive from Nextcloud WebDAV storage.
   */
  public boolean downloadBookmarkFile(@NonNull String remoteFileName, @NonNull File destinationFile)
  {
    String downloadUrl = mServerBaseUrl + DEFAULT_REMOTE_PATH + "/" + remoteFileName;
    Request req = new Request.Builder()
        .url(downloadUrl)
        .header("Authorization", mAuthHeader)
        .get()
        .build();

    try (Response resp = mHttpClient.newCall(req).execute())
    {
      if (resp.isSuccessful() && resp.body() != null)
      {
        try (java.io.InputStream in = resp.body().byteStream();
             java.io.OutputStream out = new java.io.FileOutputStream(destinationFile))
        {
          byte[] buffer = new byte[8192];
          int bytesRead;
          while ((bytesRead = in.read(buffer)) != -1)
          {
            out.write(buffer, 0, bytesRead);
          }
          out.flush();
        }
        Logger.i(TAG, "Bookmark backup downloaded successfully to " + destinationFile.getAbsolutePath());
        return true;
      }
      Logger.e(TAG, "Bookmark download returned HTTP " + resp.code());
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Bookmark download network failure: " + e.getMessage());
    }
    return false;
  }
}
