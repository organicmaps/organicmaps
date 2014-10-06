package com.mapswithme.country;

import com.mapswithme.maps.guides.GuideInfo;

public class ActiveCountryTree
{
  private ActiveCountryTree() {}

  interface ActiveCountryListener extends CountryTree.BaseListener
  {
    void onCountryProgressChanged(int group, int position, long[] sizes);

    void onCountryStatusChanged(int group, int position);

    void onCountryGroupChanged(int oldGroup, int oldPosition, int newGroup, int newPosition);

    void onCountryOptionsChanged(int group, int position);
  }

  // Should be equal to values from ActiveMapsLayout::TGroup enum
  public static final int GROUP_NEW = 0;
  public static final int GROUP_OUT_OF_DATE = 1;
  public static final int GROUP_UP_TO_DATE = 2;

  public static native int getCountInGroup(int group);

  public static native int getCountryStatus(int group, int position);

  public static native String getCountryName(int group, int position);

  public static native int getCountryOptions(int group, int position);

  // returns array of two elements : local and remote size.
  public static native long[] getCountrySize(int group, int position, int options);

  public static native long[] getDownloadableCountrySize(int group, int position);

  public static native void cancelDownloading(int group, int position);

  public static native boolean isDownloadingActive(int group, int position);

  public static native void retryDownloading(int group, int position);

  public static native void downloadMap(int group, int position, int options);

  public static native void deleteMap(int group, int position, int options);

  public static native void showOnMap(int group, int position);

  public static native GuideInfo getGuideInfo(int group, int position);

  public static native void updateAll();

  public static native void cancelAll();

  public static native int addListener(ActiveCountryListener listener);

  public static native void removeListener(int slotId);
}