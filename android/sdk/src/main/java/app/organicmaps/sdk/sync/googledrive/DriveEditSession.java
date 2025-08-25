package app.organicmaps.sdk.sync.googledrive;

import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.sync.SyncManager;
import app.organicmaps.sdk.sync.SyncOpException;
import app.organicmaps.sdk.sync.engine.EditSession;
import app.organicmaps.sdk.sync.engine.LockAlreadyHeldException;
import app.organicmaps.sdk.util.log.Logger;
import java.io.IOException;
import java.time.Instant;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import okhttp3.MediaType;
import okhttp3.MultipartBody;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class DriveEditSession implements EditSession, Runnable
{
  private static final String TAG = DriveEditSession.class.getSimpleName();
  private static final int LOCKFILE_VALIDITY_DURATION_MS =
      20_000; // Must be the same on all devices, so should be fixed at first release.
  private static final int LOCKFILE_REFRESH_INTERVAL_MS = 15_000;
  private static final String LOCKFILE = ".lock";

  private final AuthHeader mAuthHeader;
  private final String mBmDirId;
  private final String mRootDirId;
  private String mLockFileId;
  private boolean mStopped = false;
  private final HandlerThread mHandlerThread;
  private final Handler mHandler;

  DriveEditSession(AuthHeader authHeader, String bmDirId, String rootDirId)
      throws LockAlreadyHeldException, SyncOpException
  {
    mAuthHeader = authHeader;
    mBmDirId = bmDirId;
    mRootDirId = rootDirId;

    mHandlerThread = new HandlerThread("GDriveSync"); // used only for renewing lockfile
    mHandlerThread.start();
    mHandler = new Handler(mHandlerThread.getLooper());

    LockfileInfo lockfileInfo = getLockfileInfo();
    long lockfileAge;
    if (lockfileInfo != null)
    {
      lockfileAge = lockfileInfo.ageMs();
      mLockFileId = lockfileInfo.id();
    }
    else
    {
      lockfileAge = Long.MAX_VALUE;
      mLockFileId = null;
    }
    if (lockfileAge < LOCKFILE_VALIDITY_DURATION_MS)
      throw new LockAlreadyHeldException(LOCKFILE_VALIDITY_DURATION_MS - lockfileAge);
    touchLockfile();
    mHandler.postDelayed(this, LOCKFILE_REFRESH_INTERVAL_MS);
  }

  ///  Lockfile renewal runnable.
  @Override
  public void run()
  {
    try
    {
      if (!mStopped)
        touchLockfile();
      if (!mStopped)
        mHandler.postDelayed(this, LOCKFILE_REFRESH_INTERVAL_MS);
    }
    catch (SyncOpException e)
    {
      Logger.w(TAG, "Error renewing lockfile", e);
      // Won't be a problem at all, because other operations are going to fail for a similar cause, too.
    }
  }

  @Override
  public void putBookmarkFile(String fileName, byte[] fileBytes, String checksum) throws SyncOpException
  {
    String existingFileId = ClientUtils.getFileId(fileName, mBmDirId, mAuthHeader);
    if (existingFileId == null)
    {
      Request uploadRequest = createUploadRequest(fileName, mBmDirId, fileBytes, SyncManager.KML_MIME_TYPE, true);
      try (Response response = SyncManager.INSTANCE.getOkHttpClient().newCall(uploadRequest).execute())
      {
        if (!response.isSuccessful())
        {
          Logger.e(TAG, "File upload failed with status code " + response.code());
          throw new SyncOpException.UnexpectedException();
        }
      }
      catch (IOException e)
      {
        Logger.e(TAG, "Error uploading kml", e);
        throw new SyncOpException.NetworkException();
      }
    }
    else
      overwriteExistingFile(existingFileId, fileBytes, SyncManager.KML_MIME_TYPE);
  }

  @Override
  public void deleteBookmarksFile(String fileName) throws SyncOpException
  {
    String fileId = ClientUtils.getFileId(fileName, mBmDirId, mAuthHeader);
    if (fileId != null) // File exists
      deleteFile(fileId);
  }

  @Override
  public void close()
  {
    mStopped = true;
    mHandler.removeCallbacks(this);
    mHandlerThread.quit();
    try
    {
      mHandlerThread.join();
    }
    catch (InterruptedException ignored)
    {}
    // Delete lockfile
    if (mLockFileId != null)
    {
      try
      {
        deleteFile(mLockFileId);
      }
      catch (SyncOpException e)
      {
        // inconsequential
        Logger.w(TAG, "Lockfile deletion failed.", e);
      }
    }
  }

  private void touchLockfile() throws SyncOpException
  {
    if (mLockFileId == null)
    {
      // Create a new file and set mLockfileId
      Request createRequest = createUploadRequest(LOCKFILE, mRootDirId, new byte[] {}, "text/plain", false);

      try (Response response = SyncManager.INSTANCE.getOkHttpClient().newCall(createRequest).execute())
      {
        if (!response.isSuccessful())
        {
          Logger.e(TAG, "Unable to create lockfile. Request failed with status code " + response.code());
          throw new SyncOpException.UnexpectedException();
        }
        mLockFileId = new JSONObject(response.body().string()).getString("id");
      }
      catch (IOException | JSONException e)
      {
        Logger.e(TAG, "Error creating lockfile.", e);
        throw e instanceof IOException ? new SyncOpException.NetworkException()
                                       : new SyncOpException.UnexpectedException();
      }
    }
    else
      // Update last modified time
      overwriteExistingFile(mLockFileId, new byte[] {}, "text/plain");
  }

  private void deleteFile(@NonNull String fileId) throws SyncOpException
  {
    String url = Uri.withAppendedPath(Uri.parse("https://www.googleapis.com/drive/v3/files"), fileId).toString();
    Request request = new Request.Builder().url(url).delete().header("Authorization", mAuthHeader.get()).build();
    try (Response response = SyncManager.INSTANCE.getOkHttpClient().newCall(request).execute())
    {
      if (!response.isSuccessful() && response.code() != 404)
      {
        Logger.e(TAG, "File deletion for file id " + fileId + " failed with response code " + response.code());
        throw new SyncOpException.UnexpectedException();
      }
    }
    catch (IOException e)
    {
      Logger.e(TAG, "File deletion for file id " + fileId + " failed.", e);
      throw new SyncOpException.NetworkException();
    }
  }

  /**
   * @return {@code null} iff lockfile absent.
   */
  @Nullable
  private LockfileInfo getLockfileInfo() throws SyncOpException
  {
    Uri searchUri =
        Uri.parse("https://www.googleapis.com/drive/v3/files")
            .buildUpon()
            .appendQueryParameter("q", "'" + mRootDirId + "' in parents and name='" + LOCKFILE + "' and trashed=false")
            .appendQueryParameter("fields", "files(id,modifiedTime)")
            .appendQueryParameter("pageSize", "1")
            .build();
    Request request =
        new Request.Builder().url(searchUri.toString()).header("Authorization", mAuthHeader.get()).build();
    try (Response response = SyncManager.INSTANCE.getOkHttpClient().newCall(request).execute())
    {
      if (!response.isSuccessful())
        throw new SyncOpException.UnexpectedException("HTTP " + response.code());

      // Response: { "files": [ { "id": string, "modifiedTime": string } ] }
      // Date header format: "Wed, 27 Aug 2025 14:54:43 GMT"

      long serverTime;
      try
      {
        serverTime = ZonedDateTime.parse(response.header("Date", null), DateTimeFormatter.RFC_1123_DATE_TIME)
                         .toInstant()
                         .toEpochMilli();
      }
      catch (Exception e)
      {
        Logger.e(TAG, "Error parsing date header", e);
        serverTime = System.currentTimeMillis();
      }

      JSONArray respArray = new JSONObject(response.body().string()).getJSONArray("files");
      if (respArray.length() == 0)
        return null;
      else
      {
        JSONObject fields = respArray.getJSONObject(0); // modifiedTime: RFC3339
        long modifiedTimeMs = Instant.parse(fields.getString("modifiedTime")).toEpochMilli();
        return new LockfileInfo(fields.getString("id"), serverTime - modifiedTimeMs);
      }
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error trying to fetch lockfile info", e);
      throw new SyncOpException.NetworkException();
    }
    catch (JSONException e)
    {
      Logger.e(TAG, "Error trying to parse lockfile info", e);
      throw new SyncOpException.UnexpectedException(e.getLocalizedMessage());
    }
  }

  /**
   * @param customProp Whether to set appProperties field DriveClient.APP_PROPERTY_KEY
   * @return File upload request with fields=id
   */
  private Request createUploadRequest(String fileName, String parentDirId, byte[] fileBytes, String mimeType,
                                      boolean customProp) throws SyncOpException
  {
    String metadata =
        "{\"name\":" + JSONObject.quote(fileName) + ",\"parents\":[" + JSONObject.quote(parentDirId) + "]";
    if (customProp)
      metadata += ",\"appProperties\":{" + JSONObject.quote(DriveClient.APP_PROPERTY_KEY) + ":1}";
    metadata += "}";
    return new Request.Builder()
        .url("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart&fields=id")
        .post(new MultipartBody.Builder()
                  .setType(MultipartBody.MIXED)
                  .addPart(RequestBody.create(metadata, MediaType.get("application/json")))
                  .addPart(RequestBody.create(fileBytes, MediaType.get(mimeType)))
                  .build())
        .header("Authorization", mAuthHeader.get())
        .build();
  }

  private void overwriteExistingFile(@NonNull String fileId, byte[] fileBytes, String mimeType) throws SyncOpException
  {
    // This automatically updates the modifiedTime
    String url = Uri.parse("https://www.googleapis.com/upload/drive/v3/files")
                     .buildUpon()
                     .appendPath(fileId)
                     .appendQueryParameter("uploadType", "media")
                     .build()
                     .toString();
    Request updateRequest = new Request.Builder()
                                .url(url)
                                .patch(RequestBody.create(fileBytes, MediaType.get(mimeType)))
                                .header("Authorization", mAuthHeader.get())
                                .build();

    try (Response response = SyncManager.INSTANCE.getOkHttpClient().newCall(updateRequest).execute())
    {
      if (!response.isSuccessful())
      {
        Logger.e(TAG, "Error overwriting file, status code " + response.code());
        throw new SyncOpException.UnexpectedException();
      }
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error overwriting file", e);
      throw new SyncOpException.NetworkException();
    }
  }

  /**
   * @param ageMs milliseconds since last locked.
   */
  record LockfileInfo(String id, long ageMs) {}
}
