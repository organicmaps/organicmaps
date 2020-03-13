package com.mapswithme.maps.bookmarks.data;

import androidx.annotation.NonNull;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class ElevationInfo
{
  private final long mId;
  @NonNull
  private final String mName;
  @NonNull
  private final List<Point> mPoints;
  private final int mAscent;
  private final int mDescent;
  private final int mMinAltitude;
  private final int mMaxAltitude;
  private final int mDifficulty;
  private final long m_duration;

  public ElevationInfo(long trackId, @NonNull String name, @NonNull Point[] points,
                       int ascent, int descent, int minAltitude, int maxAltitude, int difficulty,
                       long m_duration)
  {
    mId = trackId;
    mName = name;
    mPoints = Arrays.asList(points);
    mAscent = ascent;
    mDescent = descent;
    mMinAltitude = minAltitude;
    mMaxAltitude = maxAltitude;
    mDifficulty = difficulty;
    this.m_duration = m_duration;
  }

  public long getId()
  {
    return mId;
  }

  @NonNull
  public String getName()
  {
    return mName;
  }

  @NonNull
  public List<Point> getPoints()
  {
    return Collections.unmodifiableList(mPoints);
  }

  public int getAscent()
  {
    return mAscent;
  }

  public int getDescent()
  {
    return mDescent;
  }

  public int getMinAltitude()
  {
    return mMinAltitude;
  }

  public int getMaxAltitude()
  {
    return mMaxAltitude;
  }

  public int getDifficulty()
  {
    return mDifficulty;
  }

  public long getM_duration()
  {
    return m_duration;
  }

  public static class Point
  {
    private final double mDistance;
    private final int mAltitude;

    public Point(double distance, int altitude)
    {
      mDistance = distance;
      mAltitude = altitude;
    }

    public double getDistance()
    {
      return mDistance;
    }

    public int getAltitude()
    {
      return mAltitude;
    }
  }
}
