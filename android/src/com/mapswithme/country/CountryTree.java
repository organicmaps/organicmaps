package com.mapswithme.country;

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

  public static native CountryItem getChildItem(int position);

  public static native void downloadCountry(int position, int options);

  public static native void deleteCountry(int position, int options);

  public static native void cancelDownloading(int position);

  public static native void retryDownloading(int position);

  public static native void showLeafOnMap(int position);

  public static native long getLeafSize(int position, int options, boolean isLocal);

  public static native void setListener(CountryTreeListener listener);

  public static native void resetListener();
}