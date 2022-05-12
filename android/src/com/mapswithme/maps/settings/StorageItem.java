package com.mapswithme.maps.settings;

/**
 * Represents storage option.
 */
public class StorageItem
{
  // Path to the root of writable directory.
  private final String mPath;
  // Free size.
  private final long mFreeSize;
  // Total size.
  private final long mTotalSize;
  // User-visible description.
  private final String mLabel;
  // Is it read-only storage?
  private final boolean mReadonly;

  public StorageItem(String path, long freeSize, long totalSize, final String label, boolean isReadonly)
  {
    mPath = path;
    mFreeSize = freeSize;
    mTotalSize = totalSize;
    mLabel = label;
    mReadonly = isReadonly;
  }

  @Override
  public boolean equals(Object o)
  {
    if (o == this)
      return true;
    if (o == null || !(o instanceof StorageItem))
      return false;
    StorageItem other = (StorageItem) o;
    // Storage equal is considered equal, either its path OR size equals to another one's.
    // Size of storage free space can change dynamically, so that hack provides us with better results identifying the same storages.
    return mFreeSize == other.getFreeSize() || mPath.equals(other.getFullPath());
  }

  @Override
  public int hashCode()
  {
    // Yes, do not put StorageItem to Hash containers, performance will be awful.
    // At least such hash is compatible with hacky equals.
    return 0;
  }

  public String getFullPath()
  {
    return mPath;
  }

  public long getFreeSize()
  {
    return mFreeSize;
  }

  public long getTotalSize()
  {
    return mTotalSize;
  }

  public String getLabel()
  {
    return mLabel;
  }

  public boolean isReadonly()
  {
    return mReadonly;
  }
}
