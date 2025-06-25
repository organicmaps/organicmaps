package app.organicmaps.sync;

import androidx.annotation.Nullable;
import java.io.File;
import java.util.HashMap;

public abstract class SyncClient
{
  public SyncClient(AuthState authState) {}

  public abstract String computeLocalFileChecksum(String filePath);

  public abstract String computeLocalFileChecksum(byte[] fileBytes);

  /**
   * The bookmarks dir state is anything that changes after every write/delete inside the cloud bookmarks
   * directory.
   * <p>
   * The {@link Syncer} is not responsible for explicitly updating it. The SyncClient must either
   * update the state itself or (preferred) rely on server-stored stored metadata like ETag or last
   * modification time.
   * <p>
   * Note that relying on modification time for self-hosted backends (e.g. Nextcloud) is unsound because
   * of server time misconfigurations, and the use of ETag is preferred.
   *
   * @return The bookmarks dir state if the directory previously existed, or {@code null} if newly created.
   */
  @Nullable
  public abstract String fetchBookmarksDirState() throws SyncOpException;

  /**
   * @return The filename-to-checksum map of each bookmark file on the server. Neither
   * the keys nor the values be null.
   * <p>
   * For backends where checksum can't be determined for
   * files not uploaded from Organic Maps, return a sentinel value for such files.
   * See {@link #isChecksumSentinel}
   */
  public abstract HashMap<String, String> fetchBmFilesStateMap() throws SyncOpException;

  /**
   * Blocks until complete.
   * @param fileName Name of the bookmark file to download.
   * @param destinationFile File to write to. Overwrites any existing file if present.
   */
  public abstract void downloadBookmarkFile(String fileName, File destinationFile) throws SyncOpException;

  public abstract EditSession getEditSession() throws Syncer.LockAlreadyHeldException, SyncOpException;

  /**
   * The inner class to perform write operations in the cloud directory.
   * Attains a lock when instantiated, which must be removed inside {@link #close()}.
   */
  public abstract static class EditSession implements AutoCloseable
  {
    /// Must be called on a thread with an active Looper
    public EditSession() {}

    /**
     * Blocks until complete.
     * @param fileName must be the file name of the bookmark file, not the entire file path.
     */
    public abstract void putBookmarkFile(String fileName, byte[] fileBytes, String checksum) throws SyncOpException;

    public abstract void deleteBookmarksFile(String fileName) throws SyncOpException;

    /**
     * Should only be overridden for backends where checksum can't be determined
     * for files not uploaded from Organic Maps (i.e. user uploaded files).
     * @see #isChecksumSentinel(String)
     */
    public void explicitlySetChecksum(String fileName, String checksum) {}

    @Override
    public abstract void close();
  }

  /**
   * Should only be overridden for backends where checksum can't be determined
   * for files not uploaded from Organic Maps (i.e. user uploaded files).
   * @see #fetchBmFilesStateMap()
   */
  public boolean isChecksumSentinel(String checksum)
  {
    return false;
  }
}
