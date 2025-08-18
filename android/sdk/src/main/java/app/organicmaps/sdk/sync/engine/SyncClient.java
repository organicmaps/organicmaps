package app.organicmaps.sdk.sync.engine;

import androidx.annotation.Nullable;
import app.organicmaps.sdk.sync.AuthState;
import app.organicmaps.sdk.sync.CloudFilesState;
import app.organicmaps.sdk.sync.SyncOpException;
import java.io.File;

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
   * See {@link CloudFilesState}
   */
  public abstract CloudFilesState fetchCloudFilesState() throws SyncOpException;

  /**
   * Blocks until complete.
   * @param fileName Name of the bookmark file to download.
   * @param destinationFile File to write to. Overwrites any existing file if present.
   */
  public abstract void downloadBookmarkFile(String fileName, File destinationFile) throws SyncOpException;

  public abstract EditSession getEditSession() throws LockAlreadyHeldException, SyncOpException;

  /**
   * The inner class to perform write operations in the cloud directory.
   * Attains a lock when instantiated, which must be removed inside {@link #close()}.
   */
  public abstract static class EditSession implements AutoCloseable
  {
    public EditSession() {}

    /**
     * Blocks until complete.
     * @param fileName must be the file name of the bookmark file, not the entire file path.
     */
    public abstract void putBookmarkFile(String fileName, byte[] fileBytes, String checksum) throws SyncOpException;

    /**
     * Can be called for OM-uploaded as well as user-uploaded files.
     */
    public abstract void deleteBookmarksFile(String fileName) throws SyncOpException;

    @Override
    public abstract void close();
  }
}
