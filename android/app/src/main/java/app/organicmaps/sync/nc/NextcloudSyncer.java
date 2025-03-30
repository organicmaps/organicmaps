package app.organicmaps.sync.nc;

import android.content.Context;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.provider.Settings;

import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.thegrizzlylabs.sardineandroid.impl.SardineException;

import app.organicmaps.util.ConnectionState;
import app.organicmaps.util.log.Logger;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArraySet;

public enum NextcloudSyncer
{
  INSTANCE;

  private static final String TAG = NextcloudSyncer.class.getSimpleName();
  private static final long SYNC_DELAY_MS = 10_000; // TODO implement exponential backoff maybe?

  final PollHelper mPollHelper;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private NextcloudPreferences ncprefs;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private CopyOnWriteArraySet<String> changedFilesSet; // thread-safe, persistent
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private String deviceName;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private AuthExpiryDetectedCallback mAuthExpiryDetectedCallback;

  private File bookmarksDir;
  private Set<String> allLocalFilePaths = new HashSet<>();

  @Nullable
  private DavClient mDavClient;

  private NextcloudSyncer()
  {
    HandlerThread handlerThread = new HandlerThread("NcSyncerThread");
    handlerThread.start();
    mPollHelper = new PollHelper(SYNC_DELAY_MS, new Handler(handlerThread.getLooper()), this::performSync);
  }

  private void saveChangedFiles()
  {
    HashSet<String> newSet = new HashSet<>(changedFilesSet);
    ncprefs.setChangedFiles(newSet);
  }

  /**
   * @return whether successful
   */
  private boolean performSync()
  {
    if (!ConnectionState.INSTANCE.isConnected() || mDavClient == null)
      return false; // TODO add options for syncing only over wifi to avoid violating the "No unexpected mobile data charges" rule
    Logger.d(TAG, "performSync started");

    String remoteBaseDirEtag;
    try
    {
      remoteBaseDirEtag = mDavClient.getBaseDirETag();
    } catch (SardineException e)
    {
      if (e.getStatusCode() == HttpURLConnection.HTTP_NOT_FOUND)
      {
        try
        {
          mDavClient.createBaseDir();
          return performSync();
        } catch (IOException ioe)
        {
          Logger.e(TAG, "Unable to create remote base folder", ioe);
        }
        return false;
      }
      else if (e.getStatusCode() == HttpURLConnection.HTTP_UNAUTHORIZED)
      {
        ncprefs.clearAuthCredentials();
        new Handler(Looper.getMainLooper()).post(() -> mAuthExpiryDetectedCallback.onDetectAuthExpiration());
        return false;
      }
      else
        Logger.e(TAG, "Unable to reach the nextcloud server", e);
      return false;
    } catch (Exception e)
    {
      Logger.e(TAG, "Unable to reach the nextcloud server", e);
      return false;
    }

    boolean syncRemoteFilesNeeded = false;
    if (!changedFilesSet.isEmpty())
    {     // there are local changes, so begin the upload process
      ArrayList<String> pendingLocalFiles = new ArrayList<>(changedFilesSet);
      for (String localFile : pendingLocalFiles)
      {
        try
        {
          syncPendingLocalFile(localFile);
          changedFilesSet.remove(localFile);
          saveChangedFiles();
        } catch (Exception e)
        {
          Logger.e(TAG, "Error syncing local file to the server. Halting sync", e);
          return false;
        }
      }
      syncRemoteFilesNeeded = true;
    }
    else
    {     // there are no local changes. Check whether there are remote changes the client needs to reconcile with.
      final String expectedBaseDirETag = getLastKnownBaseDirETag();
      if (!remoteBaseDirEtag.equals(expectedBaseDirETag))
        syncRemoteFilesNeeded = true;
    }
    if (syncRemoteFilesNeeded)
    {
      try
      {
        syncRemoteFiles();
      } catch (Exception e)
      {
        Logger.e(TAG, "Error syncing remote files with the client. Halting sync", e);
        return false;
      }
    }
    return true;
  }

  /**
   * Carries out the part-1 (i.e. uploads) of the syncing process on a per-file basis
   */
  private void syncPendingLocalFile(final String filePath) throws Exception
  {
    final File localFile = new File(filePath);
    final String remoteUrl = mDavClient.getRemoteUrl(localFile);
    final boolean localFileExists = localFile.exists();
    String remoteFileETag = null; // if it remains null after the following try-catch block, it means the remote files doesn't exist
    try
    {
      remoteFileETag = mDavClient.getFileETag(remoteUrl);
    } catch (SardineException e)
    {
      if (e.getStatusCode() != HttpURLConnection.HTTP_NOT_FOUND)
        throw e;
    }
    final boolean remoteFileExists = remoteFileETag != null;

    if (!localFileExists && !remoteFileExists)
    {
      // the file exists on neither client nor server. Some other device might have deleted this list from the server already. Update the last-known state.
      ncprefs.removeFileETag(filePath);
      return;
    }

    if (localFileExists && !remoteFileExists)
    {
      if (ncprefs.getFileETag(filePath) != null)
      {         // conflict: client expected remote file to be present (last-known), but it isn't. Adding a suffix to the local file name, and uploading it.
        final File newFile = LocalFileUtils.safeAddSuffixToFilename(localFile, null);
        final String newRemoteUrl = mDavClient.getRemoteUrl(newFile);
        mDavClient.uploadFileToRemoteURL(newFile, newRemoteUrl);
        ncprefs.removeFileETag(filePath);
        final String resultETag = mDavClient.getFileETag(newRemoteUrl);
        ncprefs.setFileETag(newRemoteUrl, Objects.requireNonNull(resultETag));
      }
      else
      {         // no conflict, just upload the file as usual.
        mDavClient.uploadFileToRemoteURL(localFile, remoteUrl);
        final String resultETag = mDavClient.getFileETag(remoteUrl);
        ncprefs.setFileETag(filePath, Objects.requireNonNull(resultETag));
      }
      return;
    }

    // Now, remoteFileExists is guaranteed to be true

    if (!localFileExists)
    {
      final String expectedETag = ncprefs.getFileETag(filePath);
      if (remoteFileETag.equals(expectedETag))
      {       // no conflict here: last-known state of remote file matches the current state, so simply delete the remote file
        mDavClient.deleteFileAtRemoteURL(remoteUrl);
        ncprefs.removeFileETag(filePath);
      }
      else
      {       // conflict: locally, the file was deleted. But the remote-file isn't in the last-known state, so it cannot be safely deleted. Nothing to do here.
        ncprefs.removeFileETag(filePath);  // so that later, the server-side copy can be downloaded.
      }
    }
    else      // both remote as well as local files exist
    {
      final String expectedETag = ncprefs.getFileETag(filePath);
      if (expectedETag == null)
      {       // The same filename exists on the server, but it wasn't previously known to have existed. Among other cases, this is the case of
        //   initial synchronization, so it'd be wise to add the device name to the local file
        final File newFile = LocalFileUtils.safeAddSuffixToFilename(localFile, "_" + deviceName);
        final String newRemoteUrl = mDavClient.getRemoteUrl(newFile);
        mDavClient.uploadFileToRemoteURL(newFile, newRemoteUrl);
        final String resultETag = mDavClient.getFileETag(newRemoteUrl);
        ncprefs.setFileETag(newFile.getAbsolutePath(), Objects.requireNonNull(resultETag));
      }
      else if (expectedETag.equals(remoteFileETag))
      {       // This is the most trivial case: a file was modified locally, and its server copy is still in the last-known state. No conflict.
        mDavClient.uploadFileToRemoteURL(localFile, remoteUrl);
        final String resultETag = mDavClient.getFileETag(remoteUrl);
        ncprefs.setFileETag(filePath, Objects.requireNonNull(resultETag));
      }
      else
      {       // conflict: the server and the local copies are in dissonance, but both are valid. So rename the local file.
        final File newFile = LocalFileUtils.safeAddSuffixToFilename(localFile, null);
        final String newRemoteUrl = mDavClient.getRemoteUrl(newFile);
        mDavClient.uploadFileToRemoteURL(newFile, newRemoteUrl);
        ncprefs.removeFileETag(filePath);  // so that the file can be downloaded in part 2 (i.e. the downloading part) of the sync
        final String resultETag = mDavClient.getFileETag(newRemoteUrl);
        ncprefs.setFileETag(newRemoteUrl, Objects.requireNonNull(resultETag));
      }
    }
  }

  /**
   * Carries out the entire part-2 (i.e. downloads) of the syncing process
   */
  private void syncRemoteFiles() throws Exception
  {
    for (DavClient.FileMetadata metadata : mDavClient.listAllFiles())
    {
      String filepath = new File(bookmarksDir, metadata.name()).getAbsolutePath();
      if (!metadata.eTag().equals(ncprefs.getFileETag(filepath)))
      {  // the file needs to be updated
        try (InputStream inputStream = mDavClient.downloadFile(mDavClient.getRemoteUrl(metadata.name()));
             BufferedInputStream bis = new BufferedInputStream(inputStream);
             FileOutputStream fos = new FileOutputStream(filepath);
             BufferedOutputStream bos = new BufferedOutputStream(fos))
        {
          byte[] buffer = new byte[4 * 1024];
          int bytesRead;
          while ((bytesRead = bis.read(buffer)) != -1)
          {
            bos.write(buffer, 0, bytesRead);
          }
        }
        ncprefs.setFileETag(filepath, metadata.eTag());
        LocalFileUtils.reloadBookmarksList(filepath);
      }
    }
    setLastKnownBaseDirETag(mDavClient.getBaseDirETag());
  }

  @Nullable
  private String getLastKnownBaseDirETag()
  {
    return ncprefs.getFileETag(":)"); // filePath argument can be any string as long as it is the same as the one in the setter
  }

  private void setLastKnownBaseDirETag(@NonNull String eTag)
  {
    ncprefs.setFileETag(":)", eTag); // filePath argument can be any string as long as it is the same as the one in the getter
  }

  public void initialize(@NonNull Context context, @NonNull AuthExpiryDetectedCallback authExpiryDetectedCallback)
  {
    ncprefs = new NextcloudPreferences(context);
    Set<String> changedFiles = ncprefs.getChangedFiles();
    if (changedFiles == null)
    {
      changedFilesSet = new CopyOnWriteArraySet<>(allLocalFilePaths);
      saveChangedFiles();
    }
    else
      changedFilesSet = new CopyOnWriteArraySet<>(changedFiles);
    this.mAuthExpiryDetectedCallback = authExpiryDetectedCallback;
    this.deviceName = getDeviceName(context);
  }

  private String getDeviceName(Context context)
  {
    String name = "";
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N_MR1)
    {
      try
      {
        name = Settings.Global.getString(context.getContentResolver(), Settings.Global.DEVICE_NAME);
      } catch (Exception ignored)
      {
      }
    }
    if (name.trim().isEmpty())
      name = Build.MANUFACTURER + "-" + Build.MODEL;
    return name;
  }

  public void onLogout()
  {
    changedFilesSet = new CopyOnWriteArraySet<>(allLocalFilePaths);
    saveChangedFiles();
    ncprefs.clearFileETags();
  }

  public void notifyFileDeleted(String filePath)
  {
    allLocalFilePaths.remove(filePath);
    onLocalChange(filePath);
  }

  public void notifyFileChanged(String filePath, boolean isInitiallyLoaded)
  {
    if (bookmarksDir == null)
      bookmarksDir = new File(filePath).getParentFile();
    allLocalFilePaths.add(filePath);
    if (isInitiallyLoaded)
      onLocalChange(filePath);
  }

  private void onLocalChange(String filePath)
  {
    if (filePath.isEmpty())
      return;
    if (!changedFilesSet.contains(filePath))
    {
      // TODO consider moving this to a background thread as it currently runs on the main thread
      changedFilesSet.add(filePath);
      saveChangedFiles();
    }
  }

  // should be called only after the framework has been initialized
  public void resumeSync()
  {
    if (!ncprefs.getSyncEnabled())
      return;
    if (mDavClient == null)
    {
      mDavClient = new DavClient(ncprefs.getLoginName(), ncprefs.getAppPassword(), ncprefs.getAuthenticatedServerUrl());
    }
    mPollHelper.start();
  }

  public void pauseSync()
  {
    mPollHelper.stop();
  }

  public interface AuthExpiryDetectedCallback
  {
    @MainThread
    void onDetectAuthExpiration();
  }
}
