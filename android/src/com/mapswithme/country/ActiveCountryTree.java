package com.mapswithme.country;

import com.mapswithme.maps.MapStorage.Index;

public class ActiveCountryTree
{
  private ActiveCountryTree() {}

  public interface ActiveCountryListener extends CountryTree.BaseListener
  {
    void onCountryProgressChanged(int group, int position, long[] sizes);

    void onCountryStatusChanged(int group, int position, int oldStatus, int newStatus);

    void onCountryGroupChanged(int oldGroup, int oldPosition, int newGroup, int newPosition);

    void onCountryOptionsChanged(int group, int position, int newOptions, int requestOptions);
  }

  public static class SimpleCountryTreeListener implements ActiveCountryListener
  {
    @Override
    public void onCountryProgressChanged(int group, int position, long[] sizes) {}

    @Override
    public void onCountryStatusChanged(int group, int position, int oldStatus, int newStatus) {}

    @Override
    public void onCountryGroupChanged(int oldGroup, int oldPosition, int newGroup, int newPosition) {}

    @Override
    public void onCountryOptionsChanged(int group, int position, int newOptions, int requestOptions) {}
  }

  // Should be equal to values from ActiveMapsLayout::TGroup enum
  public static final int GROUP_NEW = 0;
  public static final int GROUP_OUT_OF_DATE = 1;
  public static final int GROUP_UP_TO_DATE = 2;

  public static native int getOutOfDateCount();

  public static native int getCountInGroup(int group);

  public static int getTotalDownloadedCount()
  {
    return getCountInGroup(GROUP_OUT_OF_DATE) + getCountInGroup(GROUP_UP_TO_DATE);
  }

  public static native CountryItem getCountryItem(int group, int position);

  // returns array of two elements : local and remote size.
  public static native long getCountrySize(int group, int position, int options, boolean isLocal);

  public static native void cancelDownloading(int group, int position);

  public static native boolean isDownloadingActive();

  public static native void retryDownloading(int group, int position);

  public static native void downloadMap(int group, int position, int options);

  public static native void deleteMap(int group, int position, int options);

  public static native Index getCoreIndex(int group, int position);

  public static native void showOnMap(int group, int position);

  public static native void updateAll();

  public static native void cancelAll();

  public static native int addListener(ActiveCountryListener listener);

  public static native void removeListener(int slotId);

  public static native void downloadMapForIndex(Index index, int options);

  public static void downloadMapsForIndices(Index[] indices, int options)
  {
    if (indices == null)
      return;
    for (Index index : indices)
      downloadMapForIndex(index, options);
  }
}