package com.mapswithme.maps;

/**
 *  This class wraps android::Framework.cpp class
 *  via static utility methods
 */
public class Framework
{

  public interface OnApiPointActivatedListener
  {
    public void onApiPointActivated(boolean activated, double lat, double lon, String name, String id);
  }

  // Interface

  public static String getNameAndAddress4Point(double pixelX, double pixelY)
  {
    return nativeGetNameAndAddress4Point(pixelX, pixelY);
  }
  
  public static void passApiUrl(String url) 
  {
    nativePassApiUrl(url);
  }

  public static void setOnApiPointActivatedListener(OnApiPointActivatedListener listener)
  {
    nativeSetApiPointActivatedListener(listener);
  }

  public static void removeOnApiPointActivatedListener()
  {
    nativeRemoveApiPointActivatedListener();
  }

  public static void clearApiPoints()
  {
    nativeClearApiPoints();
  }

  // "Implementation"

  private native static void nativePassApiUrl(String url);

  private native static String nativeGetNameAndAddress4Point(double pointX, double pointY);
  
  private native static void nativeSetApiPointActivatedListener(OnApiPointActivatedListener listener);

  private native static void nativeRemoveApiPointActivatedListener();

  private native static void nativeClearApiPoints();

  private Framework() {}
}
