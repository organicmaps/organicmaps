package com.mapswithme.maps.downloader;

import java.util.Comparator;

/**
 * Class representing a single item in countries hierarchy.
 * Fields are filled by native code.
 */
@SuppressWarnings("unused")
public final class CountryItem implements Comparator<CountryItem>
{
  // Must correspond to ItemCategory in MapManager.cpp
  public static final int CATEGORY_NEAR_ME = 0;
  public static final int CATEGORY_DOWNLOADED = 1;
  public static final int CATEGORY_OTHER = 2;

  // Must correspond to ItemStatus in MapManager.cpp
  public static final int STATUS_UPDATABLE = 0;
  public static final int STATUS_DOWNLOADABLE = 1;
  public static final int STATUS_ENQUEUED = 2;
  public static final int STATUS_DONE = 3;
  public static final int STATUS_PROGRESS = 4;
  public static final int STATUS_FAILED = 5;


  public String id;
  public String parentId;

  public String name;
  public String parentName;

  public long size;

  public int childCount;
  public int totalChildCount;

  public int category;
  public int status;


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
  public int compare(CountryItem lhs, CountryItem rhs)
  {
    return lhs.name.compareTo(rhs.name);
  }
}