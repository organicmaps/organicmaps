package com.mapswithme.maps;

/**
 *  This class wrapps android::Framework.cpp class 
 *  via static utility methods
 */
public class Framework
{

  public static String getNameAndAddress4Point(double pixelX, double pixelY)
  {
    return nativeGetNameAndAddress4Point(pixelX, pixelY);
  }
  
  private native static String nativeGetNameAndAddress4Point(double pointX, double pointY);
  
  private Framework() {}
}
