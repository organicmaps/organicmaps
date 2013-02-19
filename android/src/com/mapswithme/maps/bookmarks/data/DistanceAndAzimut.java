package com.mapswithme.maps.bookmarks.data;

public class DistanceAndAzimut
{
  private String m_distance;
  private double m_athimuth;

  public String getDistance()
  {
    return m_distance;
  }

  public double getAthimuth()
  {
    return m_athimuth;
  }

  public DistanceAndAzimut(String m_distance, double m_athimuth)
  {
    super();
    this.m_distance = m_distance;
    this.m_athimuth = m_athimuth;
  }
}
