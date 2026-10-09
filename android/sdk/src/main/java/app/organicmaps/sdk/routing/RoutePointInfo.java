package app.organicmaps.sdk.routing;

import android.os.Parcel;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.VisibleForTesting;
import app.organicmaps.sdk.bookmarks.data.MapObjectData;

public final class RoutePointInfo implements MapObjectData
{
  public final RouteMarkType mMarkType;

  public final int mIntermediateIndex;

  // Called from JNI.
  @Keep
  @VisibleForTesting
  RoutePointInfo(int markType, int intermediateIndex)
  {
    switch (markType)
    {
    case 0: mMarkType = RouteMarkType.Start; break;
    case 1: mMarkType = RouteMarkType.Intermediate; break;
    case 2: mMarkType = RouteMarkType.Finish; break;
    default: throw new IllegalArgumentException("Mark type is not valid = " + markType);
    }

    mIntermediateIndex = intermediateIndex;
  }

  boolean isIntermediatePoint()
  {
    return mMarkType == RouteMarkType.Intermediate;
  }

  boolean isFinishPoint()
  {
    return mMarkType == RouteMarkType.Finish;
  }

  boolean isStartPoint()
  {
    return mMarkType == RouteMarkType.Start;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(@NonNull Parcel dest, int flags)
  {
    dest.writeInt(mMarkType.ordinal());
    dest.writeInt(mIntermediateIndex);
  }

  public static final Creator<RoutePointInfo> CREATOR = new Creator<>() {
    @Override
    @NonNull
    public RoutePointInfo createFromParcel(@NonNull Parcel in)
    {
      return new RoutePointInfo(in.readInt() /* mMarkType */, in.readInt() /* mIntermediateIndex */);
    }

    @Override
    @NonNull
    public RoutePointInfo[] newArray(int size)
    {
      return new RoutePointInfo[size];
    }
  };
}
