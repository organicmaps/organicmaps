package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.Keep;

public final class TrackStatistics
{
  private final double m_length;
  private final double m_duration;
  private final double m_ascent;
  private final double m_descent;
  private final int m_minElevation;
  private final int m_maxElevation;
  private final boolean m_hasElevationInfo;

  // Used by JNI
  @Keep
  private TrackStatistics(double length, double duration, double ascent, double descent, int minElevation,
                          int maxElevation, boolean hasElevationInfo)
  {
    m_length = length;
    m_duration = duration;
    m_ascent = ascent;
    m_descent = descent;
    m_minElevation = minElevation;
    m_maxElevation = maxElevation;
    m_hasElevationInfo = hasElevationInfo;
  }

  public double getLength()
  {
    return m_length;
  }

  public double getDuration()
  {
    return m_duration;
  }

  public double getAscent()
  {
    return m_ascent;
  }

  public double getDescent()
  {
    return m_descent;
  }

  public int getMinElevation()
  {
    return m_minElevation;
  }

  public int getMaxElevation()
  {
    return m_maxElevation;
  }

  /** Whether the elevation statistics are available for display. */
  public boolean hasElevationInfo()
  {
    return m_hasElevationInfo;
  }
}
