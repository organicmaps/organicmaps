package app.organicmaps.sdk.sync.engine;

import androidx.annotation.Nullable;
import app.organicmaps.sdk.sync.CloudFilesState;
import app.organicmaps.sdk.sync.SyncOpException;
import java.io.File;

public interface SyncClient
{
  String computeLocalFileChecksum(String filePath);

  String computeLocalFileChecksum(byte[] fileBytes);

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
  String fetchBookmarksDirState() throws SyncOpException;

  /**
   * See {@link CloudFilesState}
   */
  CloudFilesState fetchCloudFilesState() throws SyncOpException;

  /**
   * Blocks until complete.
   * @param fileName Name of the bookmark file to download.
   * @param destinationFile File to write to. Overwrites any existing file if present.
   */
  void downloadBookmarkFile(String fileName, File destinationFile) throws SyncOpException;

  EditSession getEditSession() throws LockAlreadyHeldException, SyncOpException;
}
