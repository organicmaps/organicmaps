package app.organicmaps.sdk.sync.engine;

import app.organicmaps.sdk.sync.SyncOpException; /**
 * Performs write operations in the cloud directory.
 * Attains a lock when instantiated, which must be removed inside {@link #close()}.
 */
public interface EditSession extends AutoCloseable
{
  /**
   * Blocks until complete.
   * @param fileName must be the file name of the bookmark file, not the entire file path.
   */
  void putBookmarkFile(String fileName, byte[] fileBytes, String checksum) throws SyncOpException;

  /**
   * Can be called for OM-uploaded as well as user-uploaded files.
   */
  void deleteBookmarksFile(String fileName) throws SyncOpException;

  @Override
  void close();
}
