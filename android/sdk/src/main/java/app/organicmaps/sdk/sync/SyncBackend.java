package app.organicmaps.sdk.sync;

public interface SyncBackend
{
  /**
   * @return an integer that denotes the backend type.
   * Persisted to the disk, so it must not be changed once published.
   */
  int getId();
}
