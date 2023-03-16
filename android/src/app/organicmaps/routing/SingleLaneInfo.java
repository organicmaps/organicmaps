package app.organicmaps.routing;

public class SingleLaneInfo
{
  byte[] mLane;
  boolean mIsActive;

  SingleLaneInfo(byte[] lane, boolean isActive)
  {
    mLane = lane;
    mIsActive = isActive;
  }

  @Override
  public String toString()
  {
    final int initialCapacity = 32;
    StringBuilder sb = new StringBuilder(initialCapacity);
    sb.append("Is the lane active? ").append(mIsActive).append(". The lane directions IDs are");
    for (byte i : mLane)
      sb.append(" ").append(i);
    return sb.toString();
  }
}
