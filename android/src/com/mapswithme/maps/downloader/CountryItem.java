package com.mapswithme.maps.downloader;

import android.support.annotation.NonNull;

/**
 * Class representing a single item in countries hierarchy.
 * Fields are filled by native code.
 */
@SuppressWarnings("unused")
public final class CountryItem implements Comparable<CountryItem>
{
  // Must correspond to ItemCategory in MapManager.cpp
  static final int CATEGORY_NEAR_ME = 0;
  static final int CATEGORY_DOWNLOADED = 1;
  static final int CATEGORY_AVAILABLE = 2;

  // Must correspond to NodeStatus in storage_defines.hpp
  public static final int STATUS_UNKNOWN = 0;
  public static final int STATUS_FAILED = 1;
  public static final int STATUS_DONE = 2;
  public static final int STATUS_DOWNLOADABLE = 3;
  public static final int STATUS_PROGRESS = 4;
  public static final int STATUS_ENQUEUED = 5;
  public static final int STATUS_UPDATABLE = 6;

  // Must correspond to NodeErrorCode in storage_defines.hpp
  public static final int ERROR_NONE = 0;
  public static final int ERROR_UNKNOWN = 1;
  public static final int ERROR_OOM = 2;
  public static final int ERROR_NO_INTERNET = 3;

  public final String id;
  public String parentId;

  public String name;
  public String parentName;

  public long size;
  public long totalSize;

  public int childCount;
  public int totalChildCount;

  public int category;
  public int status;
  public int errorCode;
  public boolean present;

  public int progress;

  // Internal ID for grouping under headers in the list
  int headerId;

  public CountryItem(String id)
  {
    this.id = id;
  }

  @Override
  public int hashCode()
  {
    return id.hashCode();
  }

  @Override
  public boolean equals(Object other)
  {
    if (this == other)
      return true;

    if (other == null || getClass() != other.getClass())
      return false;

    return id.equals(((CountryItem)other).id);
  }

  @Override
  public int compareTo(@NonNull CountryItem another)
  {
    int catDiff = (category - another.category);
    if (catDiff != 0)
      return catDiff;

    return name.compareTo(another.name);
  }

  public void update()
  {
    MapManager.nativeGetAttributes(this);
  }

  public static CountryItem fill(String countryId)
  {
    CountryItem res = new CountryItem(countryId);
    res.update();
    return res;
  }

  public boolean isExpandable()
  {
    return (totalChildCount > 1);
  }

  @Override
  public String toString()
  {
    return "{ id: \"" + id +
           "\", parentId: \"" + parentId +
           "\", category: \"" + category +
           "\", present: " + present +
           ", name: \"" + name +
           "\", parentName: \"" + parentName +
           "\", status: " + status +
           ", errorCode: " + errorCode +
           ", headerId: " + headerId +
           ", size: " + size +
           ", totalSize: " + totalSize +
           ", childCount: " + childCount +
           ", totalChildCount: " + totalChildCount +
           ", progress: " + progress +
           "}";
  }
}
