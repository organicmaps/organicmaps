package app.organicmaps.sync;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.HashSet;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

public class SyncState
{
  private static final String PREF_CHECKSUM_CACHE_PREFIX = "sync-state-";

  private static final String PREF_METADATA_PREFIX = "sync-metadata-";
  private static final String PREF_KEY_CHANGED_FILES = "changed";
  private static final String PREF_KEY_FILES_CACHE_OUTDATED = "changedFilesOutdated";
  private static final String PREF_KEY_ROOT_DIR_STATE = "root";

  private final SharedPreferences mMetadataPrefs;
  private final SharedPreferences mChecksumPrefs;
  private final ChecksumCalculator mChecksumCalculator;

  public SyncState(Context context, long accountId, ChecksumCalculator checksumCalculator)
  {
    mMetadataPrefs = context.getSharedPreferences(PREF_METADATA_PREFIX + accountId, Context.MODE_PRIVATE);
    mChecksumPrefs = context.getSharedPreferences(PREF_CHECKSUM_CACHE_PREFIX + accountId, Context.MODE_PRIVATE);
    mChecksumCalculator = checksumCalculator;

    if (isChangedFilesCacheOutdated())
      recreateChangedFilesCache();
  }

  private void recreateChangedFilesCache()
  {
    Set<String> allLocalFiles = SyncManager.INSTANCE.getLocalFilePaths();
    Map<String, String> cachedChecksums = getAllCachedChecksums();
    Set<String> changedFiles = new HashSet<>(allLocalFiles);
    for (Map.Entry<String, String> entry : cachedChecksums.entrySet())
    {
      if (allLocalFiles.contains(entry.getKey())
          && entry.getValue().equals(mChecksumCalculator.computeChecksum(entry.getKey())))
        changedFiles.remove(entry.getKey());
      else
        changedFiles.add(entry.getKey());
    }
    setChangedFiles(changedFiles);
    setChangedFilesCacheOutdated(false);
  }

  private boolean isChangedFilesCacheOutdated()
  {
    return mMetadataPrefs.getBoolean(PREF_KEY_FILES_CACHE_OUTDATED, true)
 || !mMetadataPrefs.contains(PREF_KEY_CHANGED_FILES);
  }

  /// **Must** be called with outdated=true when changes to bookmarks are no longer being tracked (i.e. sync disabled).
  public void setChangedFilesCacheOutdated(boolean outdated)
  {
    mMetadataPrefs.edit().putBoolean(PREF_KEY_FILES_CACHE_OUTDATED, outdated).apply();
  }

  @SuppressWarnings("unchecked")
  public Map<String, String> getAllCachedChecksums()
  {
    return (Map<String, String>) mChecksumPrefs.getAll();
  }

  public @Nullable String getCachedChecksum(String filePath)
  {
    return mChecksumPrefs.getString(filePath, null);
  }

  public void setCachedChecksum(String filePath, String checksum)
  {
    mChecksumPrefs.edit().putString(filePath, checksum).apply();
  }

  public void eraseCachedChecksum(String filePath)
  {
    mChecksumPrefs.edit().remove(filePath).apply();
  }

  public void reset()
  {
    mChecksumPrefs.edit().clear().apply();
    mMetadataPrefs.edit().clear().apply();
    recreateChangedFilesCache();
  }

  /**
   * Do not edit the returned set directly, it comes from SharedPreferences::getStringSet.
   */
  public @NonNull Set<String> getChangedFiles()
  {
    // The key should be set in all cases, due to the way SyncState is instantiated (and reset).
    return Objects.requireNonNull(mMetadataPrefs.getStringSet(PREF_KEY_CHANGED_FILES, null));
  }

  public void setChangedFiles(Set<String> filePaths)
  {
    mMetadataPrefs.edit().putStringSet(PREF_KEY_CHANGED_FILES, filePaths).apply();
  }

  @Nullable
  public String getBookmarksDirState()
  {
    return mMetadataPrefs.getString(PREF_KEY_ROOT_DIR_STATE, null);
  }

  public void setBookmarksDirState(String state)
  {
    mMetadataPrefs.edit().putString(PREF_KEY_ROOT_DIR_STATE, state).apply();
  }

  public static interface ChecksumCalculator {
    String computeChecksum(String filePath);
  }
}
