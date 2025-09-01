package app.organicmaps.sdk.sync.engine;

import android.content.Context;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.sync.CloudFilesState;
import app.organicmaps.sdk.sync.SyncAccount;
import app.organicmaps.sdk.sync.SyncManager;
import app.organicmaps.sdk.sync.SyncOpException;
import app.organicmaps.sdk.util.FileUtils;
import app.organicmaps.sdk.util.concurrency.UiThread;
import java.io.File;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

public class Syncer
{
  private static final long ASYNC_LOADING_WAIT_TIME = 5_000; // milliseconds
  private final SyncClient mSyncClient;
  private final int mAccountId; // Each account should have only one active syncer
  private final SyncState mLocalState;
  @NonNull
  private final Set<String> mChangedFiles; // locally changed files

  public Syncer(Context context, SyncAccount account)
  {
    mSyncClient = account.createSyncClient();
    mAccountId = account.getAccountId();
    mLocalState = new SyncState(context, account.getAccountId(), mSyncClient::computeLocalFileChecksum);

    Set<String> changedFilesPref = mLocalState.getChangedFiles();
    mChangedFiles = ConcurrentHashMap.newKeySet(changedFilesPref.size());
    mChangedFiles.addAll(changedFilesPref);
  }

  public void markFileChanged(String filePath)
  {
    if (mChangedFiles.add(filePath))
      mLocalState.setChangedFiles(mChangedFiles);
  }

  public void unmarkFileChanged(String filePath)
  {
    if (mChangedFiles.remove(filePath))
      mLocalState.setChangedFiles(mChangedFiles);
  }

  public void onSyncDisabled()
  {
    mLocalState.setChangedFilesCacheOutdated(true);
  }

  private String getCloudBmNameFromFilepath(String filePath)
  {
    return filePath.substring(filePath.lastIndexOf('/') + 1);
  }

  private File getLocalBmFileFromCloudName(File dir, String cloudName)
  {
    return new File(dir, cloudName);
  }

  public void performSync() throws SyncOpException, LockAlreadyHeldException
  {
    final String cloudDirState = mSyncClient.fetchBookmarksDirState();
    if (cloudDirState == null)
    {
      // Either this is the first time the cloud directory is being synced to, or the entire directory was deleted from
      // the cloud

      // Generally, if the local file was in sync with the cloud directory and the file got deleted from the server, the
      // local file is to be deleted as well. That should happen only when the cloud bookmarks dir is intact and only
      // the kml file within it is deleted.

      // If, however, the entire bookmarks directory was deleted from the cloud, we do not want each
      // unchanged-since-last-synced bookmark file from the device to be deleted as well. Thus, we mark all files as
      // not-ever-synced by clearing the local state.
      mLocalState.reset();
    }

    final boolean cloudHasChanges = !Objects.equals(cloudDirState, mLocalState.getBookmarksDirState());
    if (!cloudHasChanges && mChangedFiles.isEmpty())
      return; // Nothing to do, return early

    // To avoid incorrect cache of changed files if process is killed in between the sync process,
    // we mark changed files cache as outdated.
    mLocalState.setChangedFilesCacheOutdated(true);

    try (EditSession editSession = mSyncClient.getEditSession())
    {
      if (cloudHasChanges)
        bidirectionalSync(editSession, false);
      else
        unidirectionalSync(editSession);

      mLocalState.setBookmarksDirState(mSyncClient.fetchBookmarksDirState());
    }
    finally
    {
      mLocalState.setChangedFilesCacheOutdated(false);
    }
  }

  /**
   * Sync routine when cloud state has changed since last sync on this account from this device.
   */
  private void bidirectionalSync(EditSession editSession, boolean isRerun) throws SyncOpException
  {
    boolean asyncChanges = false; // Indicates if there are async changes (suffixing or importing user-uploaded files)
                                  // is probably underway.
    final CloudFilesState cloudFilesState = mSyncClient.fetchCloudFilesState(); // mutable
    while (!mChangedFiles.isEmpty())
    {
      final String locallyChangedFile = mChangedFiles.iterator().next();
      asyncChanges = syncChangedFile(editSession, locallyChangedFile, cloudFilesState) || asyncChanges;
      unmarkFileChanged(locallyChangedFile);
    }

    // Every file now inside mLocalState.getAllCachedChecksums() should also be
    // present in cloudFilesState.omBookmarkFiles(), unless it was deleted from
    // the cloud (with its contents being the same as the local file).
    // Hence, such a local file can be safely deleted.
    final Map<String, String> cachedChecksums = mLocalState.getAllCachedChecksums(); // immutable
    for (String filePath : cachedChecksums.keySet())
    {
      final String cloudName = getCloudBmNameFromFilepath(filePath);
      if (!cloudFilesState.omBookmarkFiles().containsKey(cloudName))
      {
        SyncManager.deleteCategorySilent(filePath);
        SyncManager.INSTANCE.notifyOtherSyncers(mAccountId, filePath);
      }
    }

    // Now we carry out the download stage: This is done in two steps:

    // Step 1 of 2. Iterate through OM-uploaded files and download any checksum-mismatch files, overwriting locals.
    File bmDir = SyncManager.INSTANCE.getBookmarksDir();
    for (Map.Entry<String, String> cloudFileEntry : cloudFilesState.omBookmarkFiles().entrySet())
    {
      final String cloudFileName = cloudFileEntry.getKey();
      final String cloudFileChecksum = cloudFileEntry.getValue();
      String localFilePath = getLocalBmFileFromCloudName(bmDir, cloudFileName).getPath();
      if (!cloudFileChecksum.equals(cachedChecksums.getOrDefault(localFilePath, null)))
      {
        File tempFile = getLocalBmFileFromCloudName(SyncManager.INSTANCE.getTempDir(), cloudFileName);
        mSyncClient.downloadBookmarkFile(cloudFileName, tempFile);
        String downloadedFileChecksum = mSyncClient.computeLocalFileChecksum(tempFile.getPath());

        // Some time has passed, so check again to make sure the final download
        // target file hasn't changed.
        // noinspection ConstantValue
        if (mChangedFiles.contains(localFilePath) && new File(localFilePath).exists()
            && !Objects.equals(mSyncClient.computeLocalFileChecksum(localFilePath), cachedChecksums.get(localFilePath)))
        {
          // The suffixed category will be uploaded the next time sync is performed.
          SyncManager.addSuffixToCategory(localFilePath);
          asyncChanges = true;
          mLocalState.eraseCachedChecksum(localFilePath); // No need to update `cachedChecksums`
        }
        if (!FileUtils.moveFile(tempFile.getPath(), localFilePath))
          throw new SyncOpException.UnexpectedException("Unable to move file " + tempFile.getPath() + " to "
                                                        + localFilePath);

        mLocalState.setCachedChecksum(localFilePath, downloadedFileChecksum);
        SyncManager.INSTANCE.notifyOtherSyncers(mAccountId, localFilePath);
        SyncManager.reloadBookmark(localFilePath);
      }
    }

    // Step 2 of 2. Download user-uploaded files and load them, and delete those files from the cloud.
    //              The deletion is done because the files, if successfully imported, will be synced later
    //              as OM-uploaded files, so there's no need of the user-uploaded files being in the cloud anymore.
    //              It is thus intended for user-uploaded files to disappear from the cloud when a device
    //              first syncs those files. They will be reuploaded by the current device as OM-uploaded
    //              files in the next bidirectionalSync call.

    for (String userFilename : cloudFilesState.userUploadedFiles())
    {
      File tempFile = getLocalBmFileFromCloudName(SyncManager.INSTANCE.getTempDir(), userFilename);
      mSyncClient.downloadBookmarkFile(userFilename, tempFile);
      // We now schedule this file to be loaded
      UiThread.run(() -> BookmarkManager.INSTANCE.loadBookmarksFile(tempFile.getPath(), true));
      editSession.deleteBookmarksFile(userFilename);
      asyncChanges = true;
      // This is a small window when if OM is closed, a user-uploaded bookmark file does not
      // get loaded despite the file being valid, and the file is deleted from the cloud.
      // TODO(savsch) this can be prevented by creating a new folder, and checking on every app start
      //   to make sure each file within it is loaded (and then deleted)
    }

    if (!isRerun && asyncChanges)
    {
      try
      {
        Thread.sleep(ASYNC_LOADING_WAIT_TIME);
        bidirectionalSync(editSession, true);
      }
      catch (InterruptedException ignored)
      {}
    }
  }

  /**
   * Syncs a file that was changed locally.
   * @param cloudFilesState is <b>mutated</b> by this method.
   * @return {@code true} iff a file is being changed asynchronously, so we should wait a while.
   */
  private boolean syncChangedFile(EditSession editSession, String locallyChangedFile, CloudFilesState cloudFilesState)
      throws SyncOpException
  {
    boolean asyncChanges = false;
    final String cloudName = getCloudBmNameFromFilepath(locallyChangedFile);
    final boolean localFileExists = new File(locallyChangedFile).exists();
    final boolean remoteFileExists = cloudFilesState.doesFileExist(cloudName);
    switch (FileExistence.get(localFileExists, remoteFileExists))
    {
    case Both ->
    {
      final String cloudSideChecksum = cloudFilesState.omBookmarkFiles().get(cloudName);
      // cloudSideChecksum is null if that file is user uploaded. In that case it is guaranteed that
      // the file is out of sync with (and in fact different from) the local file.
      final String lastSyncedChecksum = mLocalState.getCachedChecksum(locallyChangedFile);
      if (cloudSideChecksum != null && cloudSideChecksum.equals(lastSyncedChecksum))
      {
        // No conflicts. No other device has edited the file, so upload the file overwriting the original.
        final byte[] fileBytes = FileUtils.readFile(locallyChangedFile);
        if (fileBytes == null)
          throw new SyncOpException.UnexpectedException("Unable to read local file at " + locallyChangedFile);
        final String currentChecksum = mSyncClient.computeLocalFileChecksum(fileBytes);
        if (!cloudSideChecksum.equals(currentChecksum)) // This check saves unnecessary uploads when
        // the file wasn't effectively changed.
        {
          editSession.putBookmarkFile(cloudName, fileBytes, currentChecksum);
          mLocalState.setCachedChecksum(locallyChangedFile, currentChecksum);
          cloudFilesState.omBookmarkFiles().put(cloudName, currentChecksum);
        }
      }
      else
      {
        String localFileChecksum = null;
        if (cloudSideChecksum != null // The cloud-side file is OM-uploaded
            && (lastSyncedChecksum == null || SyncManager.INSTANCE.countActiveAccounts() > 1)) // multi-account
          localFileChecksum = mSyncClient.computeLocalFileChecksum(locallyChangedFile);

        if (localFileChecksum != null && localFileChecksum.equals(cloudSideChecksum))
          // This account is connected to this device for the first time, but the files are
          // in sync with the server already.
          // This might happen when, say, another account on this device synced to another
          // device and that device already has this new account logged in so the files
          // are already in sync.
          mLocalState.setCachedChecksum(locallyChangedFile, cloudSideChecksum);
        else
        {
          // The bookmark was edited on this device, and also edited in the cloud.
          // Both copies must be kept.
          // So we add a suffix to the local file/category name.
          // This makes room for the remote copy to be downloaded as-is.
          SyncManager.addSuffixToCategory(locallyChangedFile);
          asyncChanges = true;
          mLocalState.eraseCachedChecksum(locallyChangedFile);
          // Notify other syncers that this file is now gone.
          SyncManager.INSTANCE.notifyOtherSyncers(mAccountId, locallyChangedFile);
        }
      }
    }
    case OnlyLocal ->
    {
      String lastSyncedChecksum = mLocalState.getCachedChecksum(locallyChangedFile);
      // `lastSyncedChecksum` is null if the file has never been to the cloud
      final byte[] fileBytes = FileUtils.readFile(locallyChangedFile);
      if (fileBytes == null)
        throw new SyncOpException.UnexpectedException("Unable to read local file: " + locallyChangedFile);
      final String currentChecksum = mSyncClient.computeLocalFileChecksum(fileBytes);

      if (Objects.equals(currentChecksum, lastSyncedChecksum))
      {
        // The file wasn't actually changed (i.e. the changes made were reverted
        // so it's in the last synced state). And the file doesn't exist in the cloud.
        // The file was thus deleted on the cloud as it is on this device,
        // thus it should be deleted.
        if (!SyncManager.deleteCategorySilent(locallyChangedFile))
          throw new SyncOpException.UnexpectedException("Unable to delete local file: " + locallyChangedFile);
        SyncManager.INSTANCE.notifyOtherSyncers(mAccountId, locallyChangedFile);
      }
      else
      {
        // Simply upload the file. It's because the file either never existed
        // on the cloud (lastSyncedChecksum is null), or it was modified on this
        // device after it was last synced on the cloud (lastSyncedChecksum is stale).
        editSession.putBookmarkFile(cloudName, fileBytes, currentChecksum);
        mLocalState.setCachedChecksum(locallyChangedFile, currentChecksum);
        cloudFilesState.omBookmarkFiles().put(cloudName, currentChecksum);
      }
    }
    case OnlyRemote ->
    {
      final String cloudSideChecksum = cloudFilesState.omBookmarkFiles().get(cloudName);
      // cloudSideChecksum is null if the file was user-uploaded
      final String lastSyncedChecksum = mLocalState.getCachedChecksum(locallyChangedFile);

      if (cloudSideChecksum != null && cloudSideChecksum.equals(lastSyncedChecksum))
      {
        // The local file was deleted on this device, and was last sent to the cloud from this device itself.
        // Thus the cloud copy needs to be deleted.
        editSession.deleteBookmarksFile(cloudName);
        mLocalState.eraseCachedChecksum(locallyChangedFile);
        cloudFilesState.omBookmarkFiles().remove(cloudName);
      }
      // else (comment)
      // The file was deleted locally, but it was not in sync with its cloud counterpart
      // at the time it was deleted. Hence, we'll just download cloud file.
      // Since downloads will be taken care of outside this switch block, we have
      // nothing to do here.
    }
    case Neither ->
      // The file exists neither locally nor on the cloud.
      // Some other device might have deleted the file.
      mLocalState.eraseCachedChecksum(locallyChangedFile);
    }
    return asyncChanges;
  }

  /**
   * Sync routine when only the local state (and not the cloud state) has changed
   * since last sync on this account from this device.
   */
  private void unidirectionalSync(EditSession editSession) throws SyncOpException
  {
    while (!mChangedFiles.isEmpty())
    {
      final String locallyChangedFile = mChangedFiles.iterator().next();

      if (new File(locallyChangedFile).exists())
      {
        // The file needs to be uploaded to the cloud.
        final byte[] fileBytes = FileUtils.readFile(locallyChangedFile);
        if (fileBytes == null)
          throw new SyncOpException.UnexpectedException("Unable to read the local file " + locallyChangedFile);
        final String currentChecksum = mSyncClient.computeLocalFileChecksum(fileBytes);
        if (!Objects.equals(currentChecksum, mLocalState.getCachedChecksum(locallyChangedFile)))
        {
          final String cloudName = getCloudBmNameFromFilepath(locallyChangedFile);
          editSession.putBookmarkFile(cloudName, fileBytes, currentChecksum);
          mLocalState.setCachedChecksum(locallyChangedFile, currentChecksum);
        }
      }
      else if (mLocalState.getCachedChecksum(locallyChangedFile) != null)
      {
        // The file exists on the cloud and needs to be deleted.
        editSession.deleteBookmarksFile(getCloudBmNameFromFilepath(locallyChangedFile));
        mLocalState.eraseCachedChecksum(locallyChangedFile);
      }

      unmarkFileChanged(locallyChangedFile);
    }
  }
}
