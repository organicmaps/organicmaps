package com.mapswithme.maps.settings;

import com.mapswithme.util.Constants;

/**
 * Represents storage option.
 */
public class StorageItem
{
  // Path to the root of writable directory.
  public final String mPath;
  // Free size.
  public final long mFreeSize;

  StorageItem(String path, long size)
  {
    mPath = path;
    mFreeSize = size;
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
    return mFreeSize == other.mFreeSize || mPath.equals(other.mPath);
  }

  @Override
  public int hashCode()
  {
    // Yes, do not put StorageItem to Hash containers, performance will be awful.
    // At least such hash is compatible with hacky equals.
    return 0;
  }

  @Override
  public String toString()
  {
    return mPath + ", " + mFreeSize;
  }

  public String getFullPath()
  {
    return mPath + Constants.MWM_DIR_POSTFIX;
  }
}
