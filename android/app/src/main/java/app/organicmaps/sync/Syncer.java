package app.organicmaps.sync;

import android.content.Context;
import androidx.annotation.NonNull;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.util.FileUtils;
import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

public class Syncer
{
  private final SyncClient mSyncClient;
  private final long mAccountId; // Each account should have only one active syncer
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
      // not-every-synced by clearing the local state.
      mLocalState.reset();
    }

    final boolean cloudHasChanges = !Objects.equals(cloudDirState, mLocalState.getBookmarksDirState());
    if (!cloudHasChanges && mChangedFiles.isEmpty())
      return; // Nothing to do, return early

    try (SyncClient.EditSession editSession = mSyncClient.getEditSession())
    {
      mLocalState.setChangedFilesCacheOutdated(
          true); // To avoid incorrect cache of changed files if process is killed in between the sync process.
      if (cloudHasChanges)
      {
        final HashMap<String, String> cloudFilesMap = mSyncClient.fetchBmFilesStateMap(); // mutable
        while (!mChangedFiles.isEmpty())
        {
          final String locallyChangedFile = mChangedFiles.iterator().next();
          unmarkFileChanged(locallyChangedFile);
          final String cloudName = getCloudBmNameFromFilepath(locallyChangedFile);
          try
          {
            final boolean localFileExists = new File(locallyChangedFile).exists();
            final boolean remoteFileExists = cloudFilesMap.containsKey(cloudName);
            switch (FilePairExistence.get(localFileExists, remoteFileExists))
            {
              case Both ->
              {
                final String cloudSideChecksum = Objects.requireNonNull(cloudName);
                final String lastSyncedChecksum = mLocalState.getCachedChecksum(locallyChangedFile);
                if (cloudSideChecksum.equals(lastSyncedChecksum))
                {
                  // No conflicts. No other device has edited the file, so upload the file overwriting the original.
                  final byte[] fileBytes = FileUtils.readFileSafe(locallyChangedFile);
                  if (fileBytes == null)
                    throw new SyncOpException.UnexpectedException("Unable to read local file at " + locallyChangedFile);
                  final String currentChecksum = mSyncClient.computeLocalFileChecksum(fileBytes);
                  if (!cloudSideChecksum.equals(currentChecksum)) // This check saves unnecessary uploads when
                                                                  // the file wasn't effectively changed.
                  {
                    editSession.putBookmarkFile(cloudName, fileBytes, currentChecksum);
                    mLocalState.setCachedChecksum(locallyChangedFile, currentChecksum);
                    cloudFilesMap.put(cloudName, currentChecksum);
                  }
                }
                else
                {
                  // The bookmark was edited on this device, and also edited in the cloud.
                  // Both copies must be kept.
                  // So we add a suffix to the local file/category name.
                  // This makes room for the remote copy to be downloaded as-is.
                  SyncManager.addSuffixToCategory(locallyChangedFile);
                  mLocalState.eraseCachedChecksum(locallyChangedFile);
                  // Notify other syncers that this file is now gone.
                  SyncManager.INSTANCE.notifyOtherSyncers(mAccountId, locallyChangedFile);
                }
              }
              case OnlyLocal ->
              {
                String lastSyncedChecksum = mLocalState.getCachedChecksum(locallyChangedFile);
                // `lastSyncedChecksum` is null if the file has never been to the cloud
                final byte[] fileBytes = FileUtils.readFileSafe(locallyChangedFile);
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
                  cloudFilesMap.put(cloudName, currentChecksum);
                }
              }
              case OnlyRemote ->
              {
                final String cloudSideChecksum = Objects.requireNonNull(cloudFilesMap.get(cloudName)); // non null
                final String lastSyncedChecksum = mLocalState.getCachedChecksum(locallyChangedFile); // nullable

                if (cloudSideChecksum.equals(lastSyncedChecksum))
                {
                  // The local file was deleted on this device, and was last sent to the cloud from this device itself.
                  // Thus the cloud copy needs to be deleted.
                  editSession.deleteBookmarksFile(cloudName);
                  mLocalState.eraseCachedChecksum(locallyChangedFile);
                  cloudFilesMap.remove(cloudName);
                }
                // else (comment)
                // The file either never existed locally in the first place, or was deleted when it
                // was in conflict with the cloud version. Hence, we'll just download cloud file.
                // Since downloads will be taken care of outside this switch block, we have
                // nothing to do here.
              }
              case Neither
                  -> // The file exists neither locally nor on the cloud. Some other device might have deleted the file.
                mLocalState.eraseCachedChecksum(locallyChangedFile);
            }
          }
          catch (Exception e)
          {
            markFileChanged(locallyChangedFile);
            throw e;
          }
        } // now no file is marked as changed

        // Every file now inside mLocalState.getAllCachedChecksums() should also be
        // present in cloudFilesMap, unless it was deleted from the cloud (with its
        // contents being the same as the local file). Hence, such a local file can
        // be safely deleted
        final Map<String, String> cachedChecksums = mLocalState.getAllCachedChecksums(); // immutable
        for (String filePath : cachedChecksums.keySet())
        {
          final String cloudName = getCloudBmNameFromFilepath(filePath);
          if (!cloudFilesMap.containsKey(cloudName))
          {
            SyncManager.deleteCategorySilent(filePath);
            SyncManager.INSTANCE.notifyOtherSyncers(mAccountId, filePath);
          }
        }
        // Now we carry out the download stage: iterate through cloud files and download
        // any checksum-mismatch files, overwriting locals.
        File bmDir = SyncManager.INSTANCE.getBookmarksDir();
        File tempDir = new File(StorageUtils.getTempPath(MwmApplication.sInstance));
        for (Map.Entry<String, String> cloudFileEntry : cloudFilesMap.entrySet())
        {
          final String cloudFileName = cloudFileEntry.getKey();
          final String cloudFileChecksum = cloudFileEntry.getValue();
          String localFilePath = getLocalBmFileFromCloudName(bmDir, cloudFileName).getPath();
          if (!cloudFileChecksum.equals(cachedChecksums.getOrDefault(localFilePath, null)))
          {
            File tempFile = getLocalBmFileFromCloudName(tempDir, cloudFileName);
            mSyncClient.downloadBookmarkFile(cloudFileName, tempFile);
            String downloadedFileChecksum = mSyncClient.computeLocalFileChecksum(tempFile.getPath());

            // Some time has passed, so check again to make sure the final download
            // target file hasn't changed.
            // noinspection ConstantValue
            if (mChangedFiles.contains(localFilePath) && new File(localFilePath).exists()
                && !Objects.equals(mSyncClient.computeLocalFileChecksum(localFilePath),
                                   cachedChecksums.get(localFilePath)))
            {
              // The suffixed category will be uploaded the next time sync is performed.
              SyncManager.addSuffixToCategory(localFilePath);
              mLocalState.eraseCachedChecksum(localFilePath); // No need to update `cachedChecksums`
            }
            if (!FileUtils.moveFileSafe(tempFile.getPath(), localFilePath))
              throw new SyncOpException.UnexpectedException("Unable to move file " + tempFile.getPath() + " to "
                                                            + localFilePath);

            mLocalState.setCachedChecksum(localFilePath, downloadedFileChecksum);
            SyncManager.INSTANCE.notifyOtherSyncers(mAccountId, localFilePath);
            SyncManager.reloadBookmark(localFilePath);
            if (mSyncClient.isChecksumSentinel(cloudFileChecksum))
              editSession.explicitlySetChecksum(cloudFileName, downloadedFileChecksum);
          }
        }
      } // cloudHasChanges
      else
      {
        // noinspection ConstantValue
        while (!mChangedFiles.isEmpty())
        {
          final String locallyChangedFile = mChangedFiles.iterator().next();
          unmarkFileChanged(locallyChangedFile);
          try
          {
            if (new File(locallyChangedFile).exists())
            {
              // The file needs to be uploaded to the cloud.
              final byte[] fileBytes = FileUtils.readFileSafe(locallyChangedFile);
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
          }
          catch (Exception e)
          {
            markFileChanged(locallyChangedFile);
            throw e;
          }
        }
      }
      mLocalState.setBookmarksDirState(mSyncClient.fetchBookmarksDirState());
    }
    finally
    {
      mLocalState.setChangedFilesCacheOutdated(false);
    }
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

  public static class LockAlreadyHeldException extends Exception
  {
    private final long mRemainingTimeMillis;

    public LockAlreadyHeldException(long remainingTimeMillis)
    {
      mRemainingTimeMillis = remainingTimeMillis;
    }

    public long getExpectedRemainingTimeMs()
    {
      return mRemainingTimeMillis;
    }
  }

  private enum FilePairExistence
  {
    Both,
    OnlyLocal,
    OnlyRemote,
    Neither;
    public static FilePairExistence get(boolean localFileExists, boolean remoteFileExists)
    {
      if (localFileExists)
        return remoteFileExists ? Both : OnlyLocal;
      return remoteFileExists ? OnlyRemote : Neither;
    }
  }
}
