package app.organicmaps.sdk.sync.googledrive;

import android.net.Uri;
import android.util.Pair;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.sync.CloudFilesState;
import app.organicmaps.sdk.sync.SyncManager;
import app.organicmaps.sdk.sync.SyncOpException;
import app.organicmaps.sdk.sync.engine.EditSession;
import app.organicmaps.sdk.sync.engine.LockAlreadyHeldException;
import app.organicmaps.sdk.sync.engine.SyncClient;
import app.organicmaps.sdk.util.FileUtils;
import app.organicmaps.sdk.util.Utils;
import app.organicmaps.sdk.util.log.Logger;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Objects;
import java.util.Properties;
import okhttp3.Request;
import okhttp3.Response;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class DriveClient implements SyncClient
{
  private static final String TAG = SyncClient.class.getSimpleName();
  static final String APP_PROPERTY_KEY = "om"; // can be anything, but constant across releases
  private static final String ORGANIC_MAPS_DIRECTORY = "Organic Maps"; // no single quote
  private static final String BOOKMARKS_SUBFOLDER = "bookmarks"; // no single quote
  private final GoogleDriveAuth mAuthState;
  private final AuthHeader mAuthHeader;
  private String mBmDirId;
  private String mRootDirId;

  public DriveClient(GoogleDriveAuth authState)
  {
    mAuthState = authState;
    mAuthHeader = new AuthHeader(authState.getRefreshToken());
    loadCachedFolderIds();
  }

  @Override
  public CloudFilesState fetchCloudFilesState() throws SyncOpException
  {
    // Files uploaded by OM are distinguished from user uploaded files by the appProperties field APP_PROPERTY_KEY
    String baseUrl = "https://www.googleapis.com/drive/v3/files"
                   + "?q='" + Uri.encode(mBmDirId) + "' in parents and trashed=false"
                   + "&fields=files(name,sha1Checksum,appProperties(" + Uri.encode(APP_PROPERTY_KEY)
                   + ")),nextPageToken"
                   + "&pageSize=500";

    String nextPageToken = null;
    ArrayList<JSONArray> filesPages = new ArrayList<>();
    do
    {
      String url = baseUrl.concat(nextPageToken == null ? "" : "pageToken=" + Uri.encode(nextPageToken));
      Request listFiles = new Request.Builder().url(url).header("Authorization", mAuthHeader.get()).build();

      try (Response response = SyncManager.INSTANCE.getOkHttpClient().newCall(listFiles).execute())
      {
        if (!response.isSuccessful())
          throw new SyncOpException.UnexpectedException("HTTP " + response.code());

        // response format:
        // {
        //     "files": [
        //         {
        //             "name": "userUploadedFile.gpx",
        //             "sha1Checksum": "e193866e69e336b287657570c7a6c3e11a1590f6"
        //         },
        //         {
        //             "appProperties": {
        //                 "om": "1"
        //             },
        //             "name": "OMUploadedFile.kml",
        //             "sha1Checksum": "e193866e69e336b287657570c7a6c3e11a1590f6"
        //         },
        //     "nextPageToken": string (optional)
        // }
        JSONObject resp = new JSONObject(response.body().string());
        nextPageToken = resp.has("nextPageToken") ? resp.getString("nextPageToken") : null;
        filesPages.add(resp.getJSONArray("files"));
      }
      catch (IOException e)
      {
        Logger.e(TAG, "Error trying to fetch bookmark files list", e);
        throw new SyncOpException.NetworkException();
      }
      catch (JSONException e)
      {
        Logger.e(TAG, "Error trying to parse bookmark files response", e);
        throw new SyncOpException.UnexpectedException(e.getLocalizedMessage());
      }
    }
    while (nextPageToken != null);
    CloudFilesState result = new CloudFilesState(new HashMap<>(), new HashSet<>());
    try
    {
      for (JSONArray files : filesPages)
      {
        int pageLength = files.length();
        for (int i = 0; i < pageLength; ++i)
        {
          JSONObject file = files.getJSONObject(i);
          String fileName = file.getString("name");
          if (!fileName.matches(SyncManager.FILENAME_REGEX))
            continue;
          if (file.has("appProperties"))
            result.omBookmarkFiles().put(fileName, file.getString("sha1Checksum"));
          else
            result.userUploadedFiles().add(fileName);
        }
      }
    }
    catch (JSONException e)
    {
      Logger.e(TAG, "Error trying to parse bookmark file list json", e);
      throw new SyncOpException.UnexpectedException(e.getLocalizedMessage());
    }
    return result;
  }

  @Override
  public String computeLocalFileChecksum(String filePath)
  {
    return Utils.bytesToHex(FileUtils.calculateFileSha1(filePath), true);
  }

  @Override
  public String computeLocalFileChecksum(byte[] fileBytes)
  {
    return Utils.bytesToHex(FileUtils.calculateSha1(fileBytes), true);
  }

  @Nullable
  @Override
  public String fetchBookmarksDirState() throws SyncOpException
  {
    return fetchBookmarksDirState(false);
  }

  @Nullable
  public String fetchBookmarksDirState(boolean isRetry) throws SyncOpException
  {
    if (mRootDirId == null || mBmDirId == null)
    {
      mRootDirId = Objects.requireNonNull(ClientUtils.fetchDirId(ORGANIC_MAPS_DIRECTORY, "root", mAuthHeader).first);
      Pair<String, Boolean> bmDirFetchResult = ClientUtils.fetchDirId(BOOKMARKS_SUBFOLDER, mRootDirId, mAuthHeader);
      mBmDirId = Objects.requireNonNull(bmDirFetchResult.first);
      cacheFolderIds();
      if (bmDirFetchResult.second == Boolean.TRUE)
        return null; // because the directory is newly created
    }

    String url = "https://www.googleapis.com/drive/v3/files?q='" + Uri.encode(mBmDirId)
               + "' in parents and trashed=false&orderBy=recency&fields=files(modifiedTime)&pageSize=1000";
    Request request = new Request.Builder().url(url).header("Authorization", mAuthHeader.get()).build();
    // Response format:
    // {    "files": [ { "modifiedTime": "2025-08-28T14:04:18.300Z" } ]    }
    try (Response response = SyncManager.INSTANCE.getOkHttpClient().newCall(request).execute())
    {
      if (response.code() == 404)
      {
        if (isRetry)
        {
          Logger.e(TAG, "Impossible error: Bookmarks directory created a moment ago does not exist.");
          throw new SyncOpException.UnexpectedException();
        }
        // invalidate cached ids
        mRootDirId = null;
        mBmDirId = null;
        return fetchBookmarksDirState(true);
      }
      if (!response.isSuccessful())
        throw new SyncOpException.UnexpectedException("HTTP " + response.code());

      try
      {
        MessageDigest md5 = MessageDigest.getInstance("MD5");
        try (InputStream is = response.body().byteStream())
        {
          byte[] buffer = new byte[8192];
          int bytesRead;
          while ((bytesRead = is.read(buffer)) != -1)
            md5.update(buffer, 0, bytesRead);
        }
        return Utils.bytesToHex(md5.digest(), true);
      }
      catch (NoSuchAlgorithmException e)
      {
        return Utils.bytesToHex(FileUtils.calculateSha1(response.body().bytes()), true);
      }
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error trying to fetch bookmark directory state", e);
      throw new SyncOpException.NetworkException();
    }
  }

  @Override
  public void downloadBookmarkFile(String fileName, File destinationFile) throws SyncOpException
  {
    String fileId = ClientUtils.getFileId(fileName, mBmDirId, mAuthHeader);
    if (fileId == null)
    {
      Logger.e(TAG, "Expected file to be present on Drive, but it's not: " + fileName);
      throw new SyncOpException.UnexpectedException();
    }
    Uri requestUri = Uri.parse("https://www.googleapis.com/drive/v3/files")
                         .buildUpon()
                         .appendPath(fileId)
                         .appendQueryParameter("alt", "media")
                         .build();

    Request request =
        new Request.Builder().url(requestUri.toString()).header("Authorization", mAuthHeader.get()).build();
    try (Response response = SyncManager.INSTANCE.getOkHttpClient().newCall(request).execute())
    {
      if (!response.isSuccessful())
        throw new SyncOpException.UnexpectedException("HTTP " + response.code());

      try (InputStream inputStream = response.body().byteStream();
           OutputStream outputStream = new FileOutputStream(destinationFile, false))
      {
        byte[] buffer = new byte[8192];
        int bytesRead;
        while ((bytesRead = inputStream.read(buffer)) != -1)
          outputStream.write(buffer, 0, bytesRead);
        outputStream.flush();
      }
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error downloading bookmark file to " + destinationFile.getPath(), e);
      throw new SyncOpException.NetworkException();
    }
  }

  @Override
  public EditSession getEditSession() throws LockAlreadyHeldException, SyncOpException
  {
    if (mBmDirId == null || mRootDirId == null)
      throw new IllegalStateException("fetchBookmarksDirState must be called at least once before getEditSession");
    return new DriveEditSession(mAuthHeader, mBmDirId, mRootDirId);
  }

  private void cacheFolderIds()
  {
    if (mRootDirId == null || mBmDirId == null)
      return;

    Properties props = new Properties();
    props.setProperty(ORGANIC_MAPS_DIRECTORY, mRootDirId);
    props.setProperty(BOOKMARKS_SUBFOLDER, mBmDirId);

    try (FileOutputStream out = new FileOutputStream(ClientUtils.getPropCacheFile(mAuthState.getSubject()), false))
    {
      props.store(out, null);
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Unable to cache folder id props", e);
    }
  }

  private void loadCachedFolderIds()
  {
    Properties properties = new Properties();
    File file = ClientUtils.getPropCacheFile(mAuthState.getSubject());
    try
    {
      if (file.exists())
      {
        try (FileInputStream in = new FileInputStream(file))
        {
          properties.load(in);
        }
      }
      mRootDirId = properties.getProperty(ORGANIC_MAPS_DIRECTORY);
      mBmDirId = properties.getProperty(BOOKMARKS_SUBFOLDER);
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Unable to load cached folder ids", e);
    }
  }
}
