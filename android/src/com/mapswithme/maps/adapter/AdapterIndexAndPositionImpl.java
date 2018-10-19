package com.mapswithme.maps.adapter;

final class AdapterIndexAndPositionImpl implements AdapterIndexAndPosition
{
  private final int mIndex;
  private final int mRelativePosition;

  AdapterIndexAndPositionImpl(int index, int relativePosition)
  {
    mIndex = index;
    mRelativePosition = relativePosition;
  }

  @Override
  public int getRelativePosition()
  {
    return mRelativePosition;
  }

  @Override
  public int getIndex()
  {
    return mIndex;
  }

  @Override
  public String toString()
  {
    final StringBuilder sb = new StringBuilder("AdapterIndexAndPositionImpl{");
    sb.append("mIndex=").append(mIndex);
    sb.append(", mRelativePosition=").append(mRelativePosition);
    sb.append('}');
    return sb.toString();
  }
}
