package com.mapswithme.maps.adapter;

final class AdapterIndexAndViewTypeImpl implements AdapterIndexAndViewType
{
  private final int mIndex;
  private final int mViewType;

  AdapterIndexAndViewTypeImpl(int index, int viewType)
  {
    mIndex = index;
    mViewType = viewType;
  }

  @Override
  public int getRelativeViewType()
  {
    return mViewType;
  }

  @Override
  public int getIndex()
  {
    return mIndex;
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    AdapterIndexAndViewTypeImpl that = (AdapterIndexAndViewTypeImpl) o;

    if (mIndex != that.mIndex) return false;
    return mViewType == that.mViewType;
  }

  @Override
  public int hashCode()
  {
    int result = mIndex;
    result = 31 * result + mViewType;
    return result;
  }

  @Override
  public String toString()
  {
    final StringBuilder sb = new StringBuilder("AdapterIndexAndViewTypeImpl{");
    sb.append("mIndex=").append(mIndex);
    sb.append(", mViewType=").append(mViewType);
    sb.append('}');
    return sb.toString();
  }
}
