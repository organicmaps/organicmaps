package com.mapswithme.maps;

/**
 *  This class wrapps android::Framework.cpp class 
 *  via static utility methods
 */
public class Framework
{

  public static String getNameAndAddress4Point(double mercatorX, double mercatorY)
  {
    return nativeGetNameAndAddress4Point(mercatorX, mercatorY);
  }
  
  private native static String nativeGetNameAndAddress4Point(double mercatorX, double mercatorY);
  
  private Framework() {}
}
