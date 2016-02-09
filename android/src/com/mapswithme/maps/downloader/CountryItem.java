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
  public static final int CATEGORY_NEAR_ME = 0;
  public static final int CATEGORY_DOWNLOADED = 1;
  public static final int CATEGORY_ALL = 2;

  // Must correspond to NodeStatus in storage_defines.hpp
  public static final int STATUS_UNKNOWN = 0;
  public static final int STATUS_FAILED = 1;
  public static final int STATUS_DONE = 2;
  public static final int STATUS_DOWNLOADABLE = 3;
  public static final int STATUS_PROGRESS = 4;
  public static final int STATUS_ENQUEUED = 5;
  public static final int STATUS_UPDATABLE = 6;
  public static final int STATUS_MIXED = 7;

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

  public int progress;

  // Internal ID for grouping under headers in the list
  public int headerId;

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

  public static CountryItem fill(String countryId)
  {
    CountryItem res = new CountryItem(countryId);
    MapManager.nativeGetAttributes(res);
    return res;
  }

  @Override
  public String toString()
  {
    return "{ id: \"" + id +
           "\", parentId: \"" + parentId +
           "\", category: " + category +
           ", status: " + status +
           ", headerId: " + headerId +
           ", name: \"" + name +
           "\", parentName: \"" + parentName +
           "\", size: " + size +
           ", totalSize: " + totalSize +
           ", childCount: " + childCount +
           ", totalChildCount: " + totalChildCount +
           "}";
  }
}
