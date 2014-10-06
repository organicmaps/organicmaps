package com.mapswithme.country;

import com.mapswithme.maps.guides.GuideInfo;

public class CountryTree
{
  // interface for listening callbacks from native
  public interface BaseListener {}

  public interface CountryTreeListener extends BaseListener
  {
    void onItemProgressChanged(int position, long[] sizes);

    void onItemStatusChanged(int position);
  }

  private CountryTree() {}

  public static native void setDefaultRoot();

  public static native void setParentAsRoot();

  public static native void setChildAsRoot(int position);

  public static native void resetRoot();

  public static native boolean hasParent();

  public static native int getChildCount();

  public static native boolean isLeaf(int position);

  public static native String getChildName(int position);

  public static native int getLeafStatus(int position);

  public static native int getLeafOptions(int position);

  public static native void downloadCountry(int position, int options);

  public static native void deleteCountry(int position, int options);

  public static native void cancelDownloading(int position);

  public static native void showLeafOnMap(int position);

  public static native GuideInfo getLeafGuideInfo(int position);

  public static native long[] getDownloadableLeafSize(int position);

  public static native long[] getLeafSize(int position, int options);

  public static native void setListener(CountryTreeListener listener);

  public static native void resetListener();
}