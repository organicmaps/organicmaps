package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.widget.placepage.PlacePageData;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

// Used by JNI.
@Keep
@SuppressWarnings("unused")
public class ElevationInfo implements PlacePageData
{
  @NonNull
  private final List<Point> mPoints;
  private final int mDifficulty;

  public ElevationInfo(@NonNull Point[] points, int difficulty)
  {
    mPoints = Arrays.asList(points);
    mDifficulty = difficulty;
  }

  protected ElevationInfo(Parcel in)
  {
    mDifficulty = in.readInt();
    mPoints = readPoints(in);
  }

  @NonNull
  private static List<Point> readPoints(@NonNull Parcel in)
  {
    List<Point> points = new ArrayList<>();
    in.readTypedList(points, Point.CREATOR);
    return points;
  }

  @NonNull
  public List<Point> getPoints()
  {
    return Collections.unmodifiableList(mPoints);
  }

  public int getDifficulty()
  {
    return mDifficulty;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeInt(mDifficulty);
    // All collections are deserialized AFTER non-collection and primitive type objects,
    // so collections must be always serialized at the end.
    dest.writeTypedList(mPoints);
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public static class Point implements Parcelable
  {
    private final double mDistance;
    private final int mAltitude;
    private final double mLatitude;
    private final double mLongitude;

    public Point(double distance, int altitude, double latitude, double longitude)
    {
      mDistance = distance;
      mAltitude = altitude;
      mLatitude = latitude;
      mLongitude = longitude;
    }

    protected Point(Parcel in)
    {
      mDistance = in.readDouble();
      mAltitude = in.readInt();
      mLatitude = in.readDouble();
      mLongitude = in.readDouble();
    }

    public static final Creator<Point> CREATOR = new Creator<>() {
      @Override
      public Point createFromParcel(Parcel in)
      {
        return new Point(in);
      }

      @Override
      public Point[] newArray(int size)
      {
        return new Point[size];
      }
    };

    public double getDistance()
    {
      return mDistance;
    }

    public int getAltitude()
    {
      return mAltitude;
    }

    public double getLatitude()
    {
      return mLatitude;
    }

    public double getLongitude()
    {
      return mLongitude;
    }

    @Override
    public int describeContents()
    {
      return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      dest.writeDouble(mDistance);
      dest.writeInt(mAltitude);
    }
  }

  public static final Creator<ElevationInfo> CREATOR = new Creator<>() {
    @Override
    public ElevationInfo createFromParcel(Parcel in)
    {
      return new ElevationInfo(in);
    }

    @Override
    public ElevationInfo[] newArray(int size)
    {
      return new ElevationInfo[size];
    }
  };
}
