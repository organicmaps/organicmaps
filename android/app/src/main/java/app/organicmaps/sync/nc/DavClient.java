package app.organicmaps.sync.nc;

import android.os.Handler;
import android.os.Looper;

import androidx.annotation.NonNull;

import com.thegrizzlylabs.sardineandroid.Sardine;
import com.thegrizzlylabs.sardineandroid.impl.OkHttpSardine;
import com.thegrizzlylabs.sardineandroid.impl.SardineException;


import experiment.InsecureHttpsHelper;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.util.Date;
import java.util.List;
import java.util.stream.Collectors;

class DavClient
{
  private static final String KML_MIME_TYPE = "application/vnd.google-earth.kml+xml";
  private static final String ROOT_DIR_PATH = "/OM-Experimental/";
  private static final String BOOKMARKS_DIR_PATH = ROOT_DIR_PATH + "bookmarks/";
  private static final int MAX_CONSECUTIVE_LOCKS = 8;
  private static final int LOCKFILE_VALIDITY_DURATION_MS = 7000;
  private static final int LOCKFILE_REFRESH_INTERVAL_MS = 5000;

  private final Sardine mSardine;
  private final String remoteRootUrl;
  private final String remoteBaseUrl;
  private int consecutiveLocks = MAX_CONSECUTIVE_LOCKS;

  public DavClient(@NonNull String username, @NonNull String password, @NonNull String remoteServerUrl)
  {
    mSardine = new OkHttpSardine(InsecureHttpsHelper.createInsecureOkHttpClient());
    mSardine.setCredentials(username, password, true); // use preemptive because nextcloud requires auth anyways, so as to avoid duplicate requests
    this.remoteRootUrl = remoteServerUrl + "/remote.php/dav/files/" + username + ROOT_DIR_PATH;
    this.remoteBaseUrl = remoteServerUrl + "/remote.php/dav/files/" + username + BOOKMARKS_DIR_PATH;
  }

  /**
   * Must be called from a looper thread. It also maintains the lock.
   * @return whether lock attained successfully
   */
  public boolean attainLock()
  {
    if (consecutiveLocks < MAX_CONSECUTIVE_LOCKS)
    {
      consecutiveLocks = 0;
      return true;
    }
    try
    {
      return mSardine.list(lockfileUrl(), 0).get(0).getModified().before(new Date(getServerTime().getTime() - LOCKFILE_VALIDITY_DURATION_MS));
    }
    catch (SardineException e)
    {
      if (e.getStatusCode() != HttpURLConnection.HTTP_NOT_FOUND)
      {
        return false;
      }
    }
    catch (IOException e)
    {
      return false;
    }
    consecutiveLocks = 0;
    new Handler(Looper.myLooper()).post(this::maintainLock);
    return true;
  }

  private void maintainLock()
  {
    try
    {
      mSardine.put(lockfileUrl(), new byte[] {});
      ++consecutiveLocks;
    } catch (IOException e)
    {
      // must be connectivity loss.
      consecutiveLocks = MAX_CONSECUTIVE_LOCKS; // give up the lock

    }
    if (consecutiveLocks < MAX_CONSECUTIVE_LOCKS)
      new Handler(Looper.myLooper()).postDelayed(this::maintainLock, LOCKFILE_REFRESH_INTERVAL_MS);
    else
      attemptLockfileDeletion();
  }

  private void attemptLockfileDeletion()
  {
    try
    {
      mSardine.delete(lockfileUrl());
    }
    catch (IOException ignored)
    {
      // doesn't matter, the lock will expire anyways
    }
  }

  /**
   * Make sure to call it from the same thread you called attainLock from.
   */
  public void releaseLock()
  {
    if (consecutiveLocks < MAX_CONSECUTIVE_LOCKS)
    {
      consecutiveLocks = MAX_CONSECUTIVE_LOCKS;
      attemptLockfileDeletion();
    }
  }

  public String getRemoteUrl(File localFile)
  {
    return getRemoteUrl(localFile.getName());
  }

  public String getRemoteUrl(String fileName)
  {
    return remoteBaseUrl + fileName;
  }

  public String getRemoteBaseUrl()
  {
    return remoteBaseUrl;
  }

  private String lockfileUrl()
  {
    return remoteRootUrl + ".lock";
  }

  public Date getServerTime() throws IOException
  {
    String ignoreFilePath = remoteRootUrl + ".syncer.ignore";
    mSardine.put(ignoreFilePath, new byte[] {});
    return mSardine.list(ignoreFilePath, 0).get(0).getModified();
  }

  public String getBaseDirETag() throws IOException
  {
    return mSardine.list(remoteBaseUrl, 0).get(0).getEtag();
  }

  public String getFileETag(String remoteUrl) throws IOException
  {
    return mSardine.list(remoteUrl).get(0).getEtag();
  }

  public void createBaseDir() throws IOException
  {
    try
    {
      mSardine.createDirectory(remoteRootUrl);
    }
    catch (Exception ignored) {}
    mSardine.createDirectory(remoteBaseUrl);
  }

  public void uploadFileToRemoteURL(File localFile, String remoteUrl) throws IOException
  {
    mSardine.put(remoteUrl, localFile, KML_MIME_TYPE);  // TODO if and when replacing sardine with handwritten code, make sure to leverage the `OC-Etag` header.
  }

  public void deleteFileAtRemoteURL(String remoteUrl) throws IOException
  {
    mSardine.delete(remoteUrl);
  }

  public InputStream downloadFile(String remoteUrl) throws IOException
  {
    return mSardine.get(remoteUrl);
  }

  /**
   * @return a list of FileMetadata corresponding to each KML file present inside the base bookmarks directory
   */
  public List<FileMetadata> listAllKmlFiles() throws Exception
  {
    return mSardine.list(remoteBaseUrl, 1).stream()
      .skip(1)
      .filter(davResource ->
        davResource.getName()
          .toLowerCase()
          .endsWith(".kml")
      )
      .map(davResource ->
        new FileMetadata(
          davResource.getName(),
          davResource.getEtag()
        )
      )
      .collect(Collectors.toList());
  }

  public record FileMetadata(String name, String eTag) {}

/*
 UNNEEDED because 1 http-request can be saved by instead getting ETag and watching out for 404
  public boolean resourceExists(String remoteUrl) throws IOException
  {
    return mSardine.exists(remoteUrl);
  }
*/
}
