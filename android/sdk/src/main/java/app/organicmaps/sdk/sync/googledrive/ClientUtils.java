package app.organicmaps.sdk.sync.googledrive;

import android.net.Uri;
import android.util.Pair;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.sync.SyncManager;
import app.organicmaps.sdk.sync.SyncOpException;
import app.organicmaps.sdk.util.log.Logger;
import java.io.File;
import java.io.IOException;
import okhttp3.MediaType;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

class ClientUtils
{
  private static final String TAG = ClientUtils.class.getSimpleName();

  /**
   * @return {@code null} iff the file doesn't exist.
   */
  @Nullable
  static String getFileId(String fileName, String parentDirId, AuthHeader authHeader) throws SyncOpException
  {
    Uri searchUri =
        Uri.parse("https://www.googleapis.com/drive/v3/files")
            .buildUpon()
            .appendQueryParameter("q", "'" + parentDirId + "' in parents and name='" + fileName + "' and trashed=false")
            .appendQueryParameter("fields", "files(id)")
            .appendQueryParameter("pageSize", "1")
            .build();
    Request request = new Request.Builder().url(searchUri.toString()).header("Authorization", authHeader.get()).build();
    try (Response response = SyncManager.INSTANCE.getOkHttpClient().newCall(request).execute())
    {
      if (!response.isSuccessful())
        throw new SyncOpException.UnexpectedException("HTTP " + response.code());

      // Response: { "files": [ { "id": string } ] }
      JSONArray respArray = new JSONObject(response.body().string()).getJSONArray("files");
      if (respArray.length() == 0)
        return null;
      else
        return respArray.getJSONObject(0).getString("id");
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error trying to fetch file id for " + fileName, e);
      throw new SyncOpException.NetworkException();
    }
    catch (JSONException e)
    {
      Logger.e(TAG, "Error trying to parse file id response for " + fileName, e);
      throw new SyncOpException.UnexpectedException(e.getLocalizedMessage());
    }
  }

  /**
   * Creates the directory iff it doesn't exist.
   * @param dirName Name of the directory to look for.
   * @param parentDirId id of the parent directory of the directory to look for.
   * @return pair(id of the specified directory, whether it was newly created).
   */
  @NonNull
  static Pair<String, Boolean> fetchDirId(String dirName, String parentDirId, AuthHeader authHeader)
      throws SyncOpException
  {
    Uri searchUri =
        Uri.parse("https://www.googleapis.com/drive/v3/files")
            .buildUpon()
            .appendQueryParameter("q", "'" + parentDirId + "' in parents and name='" + dirName + "' and trashed=false")
            .appendQueryParameter("fields", "files(id)")
            .appendQueryParameter("pageSize", "1")
            .build();

    Request request = new Request.Builder().url(searchUri.toString()).header("Authorization", authHeader.get()).build();
    try (Response response = SyncManager.INSTANCE.getOkHttpClient().newCall(request).execute())
    {
      if (!response.isSuccessful())
        throw new SyncOpException.UnexpectedException("HTTP " + response.code());

      // Example response when dir exists: { "files": [ { "id": string } ] }
      // Example response when dir doesn't exist: { "files": [] }
      JSONArray respArray = new JSONObject(response.body().string()).getJSONArray("files");
      if (respArray.length() == 0)
        return new Pair<>(createDirectory(dirName, parentDirId, authHeader), Boolean.TRUE);
      else
      {
        JSONObject fields = respArray.getJSONObject(0);
        return new Pair<>(fields.getString("id"), Boolean.FALSE);
      }
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error trying to look for directory " + dirName, e);
      throw new SyncOpException.NetworkException();
    }
    catch (JSONException e)
    {
      Logger.e(TAG, "Error trying to parse directory search response for " + dirName, e);
      throw new SyncOpException.UnexpectedException(e.getLocalizedMessage());
    }
  }

  /**
   * @param dirName Name of the directory to create.
   * @param parentDirId File-id of the parent directory to create.
   * @return file-id of the newly created directory
   */
  static String createDirectory(String dirName, String parentDirId, AuthHeader authHeader) throws SyncOpException
  {
    Request request;
    try
    {
      request = new Request.Builder()
                    .url("https://www.googleapis.com/drive/v3/files?fields=id")
                    .post(RequestBody.create(new JSONObject()
                                                 .put("name", dirName)
                                                 .put("mimeType", "application/vnd.google-apps.folder")
                                                 .put("parents", new JSONArray().put(parentDirId))
                                                 .toString(),
                                             MediaType.get("application/json")))
                    .header("Authorization", authHeader.get())
                    .build();
    }
    catch (JSONException e)
    {
      Logger.e(TAG, "Error constructing directory creation request for " + dirName, e);
      throw new SyncOpException.UnexpectedException(e.getLocalizedMessage());
    }

    try (Response response = SyncManager.INSTANCE.getOkHttpClient().newCall(request).execute())
    {
      if (!response.isSuccessful())
        throw new SyncOpException.UnexpectedException("HTTP " + response.code());

      // Sample response: { "id": "13pnFwDciJnupGVbuiRC4yMenzfBCA6rP" }
      return new JSONObject(response.body().string()).getString("id");
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error creating directory " + dirName, e);
      throw new SyncOpException.NetworkException();
    }
    catch (JSONException e)
    {
      Logger.e(TAG, "Error parsing creation response for directory " + dirName, e);
      throw new SyncOpException.UnexpectedException(e.getLocalizedMessage());
    }
  }

  /**
   *
   * @param subject Something unique to the properties. Google OAuth2 id_token's subject, for example.
   */
  static File getPropCacheFile(String subject)
  {
    String reducedSub = subject.replaceAll("[^a-zA-Z0-9]", "");
    // Each Google account "subject" so far is a 21 character numeric string.
    // Even if that weren't the case, it wouldn't be a problem. It's because
    // propCacheFile uniqueness (per-subject) is expected but isn't needed.
    return new File(SyncManager.INSTANCE.getTempDir(), reducedSub + ".drive");
  }
}
