package com.mapswithme.maps.location;

public class LocationPermissionNotGrantedException extends Exception
{
  private static final long serialVersionUID = -1053815743102694164L;

  public LocationPermissionNotGrantedException()
  {
    super("Location permission not granted!");
  }
}
